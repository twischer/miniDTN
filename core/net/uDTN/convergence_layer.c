/**
 * \addtogroup cl
 *
 * @{
 */

/**
 * \file
 * \brief IEEE 802.15.4 Convergence Layer Implementation
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <string.h> // memset

#include "contiki.h"
#include "packetbuf.h"
#include "netstack.h"
#include "rimeaddr.h"
#include "process.h"
#include "list.h"

#include "agent.h"
#include "logging.h"
#include "storage.h"
#include "discovery.h"
#include "dtn_network.h"
#include "dispatching.h"
#include "bundleslot.h"
#include "statusreport.h"
#include "bundle_ageing.h"

#include "convergence_layer.h"

/**
 * Structure to keep track of neighbours from which we are currently awaiting app-layer ACKs
 */
struct blocked_neighbour_t {
	struct blocked_neighbour_t * next;

	/* Address of the neighbour */
	rimeaddr_t neighbour;

	/* Since when is he blocked? */
	clock_time_t timestamp;
};

/**
 * List to keep track of outgoing bundles
 */
LIST(transmission_ticket_list);
MEMB(transmission_ticket_mem, struct transmit_ticket_t, CONVERGENCE_LAYER_QUEUE);

/**
 * List to keep track of blocked neighbours
 */
LIST(blocked_neighbour_list);
MEMB(blocked_neighbour_mem, struct blocked_neighbour_t, CONVERGENCE_LAYER_QUEUE);

/**
 * Internal functions
 */
int convergence_layer_is_blocked(rimeaddr_t * neighbour);
int convergence_layer_set_blocked(rimeaddr_t * neighbour);
int convergence_layer_set_unblocked(rimeaddr_t * neighbour);

/**
 * CL process
 */
PROCESS(convergence_layer_process, "CL process");

/**
 * MUTEX to avoid flooding the MAC layer with outgoing bundles
 */
int convergence_layer_transmitting;

/**
 * Keep track of the allocated tickets
 */
int convergence_layer_slots;

/**
 * Keep track of the tickets in queue
 */
int convergence_layer_queue;

/**
 * Use a unique sequence number of each outgoing segment
 */
uint8_t outgoing_sequence_number;

/**
 * Indicate when a continue event or poll to ourselves is pending to avoid
 * exceeding the event queue size 
 */
uint8_t convergence_layer_pending;

/**
 * Backoff timer
 */
struct etimer convergence_layer_backoff;
uint8_t convergence_layer_backoff_pending = 0;

void convergence_layer_show_tickets();

int convergence_layer_init(void)
{
	// Start CL process
	process_start(&convergence_layer_process, NULL);

	return 1;
}

struct transmit_ticket_t * convergence_layer_get_transmit_ticket_priority(uint8_t priority)
{
	struct transmit_ticket_t * ticket = NULL;

	if( priority == CONVERGENCE_LAYER_PRIORITY_NORMAL && (CONVERGENCE_LAYER_QUEUE - convergence_layer_slots) <= CONVERGENCE_LAYER_QUEUE_FREE ) {
		// Cannot assign ticket because too many slots are in use
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Cannot allocate ticket with low priority");
		return NULL;
	}

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Requested ticket with priority %d", priority);

	/* Allocate memory */
	ticket = memb_alloc(&transmission_ticket_mem);
	if( ticket == NULL ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Cannot allocate ticket");
		return NULL;
	}
	ticket->timestamp = clock_time();

	/* Initialize the ticket */
	memset(ticket, 0, sizeof(struct transmit_ticket_t));

	/* Add it to our list */
	if( priority == CONVERGENCE_LAYER_PRIORITY_NORMAL ) {
		/* Append to list */
		list_add(transmission_ticket_list, ticket);
	} else {
		/* Prepend to list */
		list_push(transmission_ticket_list, ticket);
	}

	/* Count the used slots */
	convergence_layer_slots++;

	/* Hand over the ticket to our app */
	return ticket;
}

struct transmit_ticket_t * convergence_layer_get_transmit_ticket()
{
	return convergence_layer_get_transmit_ticket_priority(CONVERGENCE_LAYER_PRIORITY_NORMAL);
}

int convergence_layer_free_transmit_ticket(struct transmit_ticket_t * ticket)
{
	if( ticket->bundle != NULL ) {
		bundle_decrement(ticket->bundle);
		ticket->bundle = NULL;
	}

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Freeing ticket %p", ticket);

	/* Only dequeue bundles that have been in the queue */
	if( (ticket->flags & CONVERGENCE_LAYER_QUEUE_ACTIVE) || (ticket->flags & CONVERGENCE_LAYER_QUEUE_DONE) || (ticket->flags & CONVERGENCE_LAYER_QUEUE_FAIL) ) {
		convergence_layer_queue--;
	}

	/* Count the used slots */
	convergence_layer_slots--;

	/* Remove ticket from list and free memory */
	list_remove(transmission_ticket_list, ticket);
	memset(ticket, 0, sizeof(struct transmit_ticket_t));
	memb_free(&transmission_ticket_mem, ticket);

	return 1;
}

int convergence_layer_enqueue_bundle(struct transmit_ticket_t * ticket)
{
	if( ticket == NULL ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Cannot enqueue invalid ticket %p", ticket);
		return -1;
	}

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Enqueuing bundle %lu to %u.%u, queue is at %u entries", ticket->bundle_number, ticket->neighbour.u8[0], ticket->neighbour.u8[1], convergence_layer_queue);

	/* The ticket is now active a ready for transmission */
	ticket->flags |= CONVERGENCE_LAYER_QUEUE_ACTIVE;

	if( convergence_layer_pending == 0 ) {
		/* Poll the process to initiate transmission */
		process_poll(&convergence_layer_process);
	}

	convergence_layer_queue++;

	return 1;
}

int convergence_layer_send_bundle(struct transmit_ticket_t * ticket)
{
	struct bundle_t *bundle = NULL;
	uint8_t length = 0;
	uint8_t * buffer = NULL;
	uint8_t buffer_length = 0;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Sending bundle %lu to %u.%u with ticket %p", ticket->bundle_number, ticket->neighbour.u8[0], ticket->neighbour.u8[1], ticket);

	/* Read the bundle from storage, if it is not in memory */
	if( ticket->bundle == NULL ) {
		ticket->bundle = BUNDLE_STORAGE.read_bundle(ticket->bundle_number);
		if( ticket->bundle == NULL ) {
			LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Unable to read bundle %lu", ticket->bundle_number);
			/* FIXME: Notify somebody */
			return -1;
		}
	}

	/* Get our bundle struct and check the pointer */
	bundle = (struct bundle_t *) MMEM_PTR(ticket->bundle);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Invalid bundle pointer for bundle %lu", ticket->bundle_number);
		bundle_decrement(ticket->bundle);
		ticket->bundle = NULL;
		return -1;
	}

	/* Check if bundle has expired */
	if( bundle_ageing_is_expired(ticket->bundle) ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_INF, "Bundle %lu has expired, not sending it", ticket->bundle_number);

		/* Bundle is expired */
		bundle_decrement(ticket->bundle);

		/* Tell storage to delete - it will take care of the rest */
		BUNDLE_STORAGE.del_bundle(ticket->bundle_number, REASON_LIFETIME_EXPIRED);

		return -1;
	}

	/* Get our buffer */
	buffer = dtn_network_get_buffer();
	if( buffer == NULL ) {
		bundle_decrement(ticket->bundle);
		ticket->bundle = NULL;
		return -1;
	}

	/* Get the buffer length */
	buffer_length = dtn_network_get_buffer_length();

	/* Put in the prefix */
	buffer[0]  = 0;
	buffer[0] |= CONVERGENCE_LAYER_TYPE_DATA & CONVERGENCE_LAYER_MASK_TYPE;
	buffer[0] |= (CONVERGENCE_LAYER_FLAGS_FIRST | CONVERGENCE_LAYER_FLAGS_LAST) & CONVERGENCE_LAYER_MASK_FLAGS;
	buffer[0] |= (outgoing_sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
	ticket->sequence_number = outgoing_sequence_number;
	outgoing_sequence_number = (outgoing_sequence_number + 1) % 4;
	length = 1;

	/* Encode the bundle into the buffer */
	length += bundle_encode_bundle(ticket->bundle, buffer + 1, buffer_length - 1);

	/* Check if we may violate the maximum length */
	if( length > CONVERGENCE_LAYER_MAX_LENGTH ) {
		ticket->flags = CONVERGENCE_LAYER_QUEUE_FAIL;

		/* Notify routing module */
		ROUTING.sent(ticket, ROUTING_STATUS_ERROR);

		return -1;
	}

	/* Flag the bundle as being in transit now */
	ticket->flags |= CONVERGENCE_LAYER_QUEUE_IN_TRANSIT;

	/* Now we are transmitting */
	convergence_layer_transmitting = 1;

	/* This neighbour is blocked, until we have received the App Layer ACK or NACK */
	convergence_layer_set_blocked(&ticket->neighbour);

	/* And send it out */
	dtn_network_send(&ticket->neighbour, length, (void *) ticket);

	return 1;
}

int convergence_layer_send_discovery(uint8_t * payload, uint8_t length, rimeaddr_t * neighbour)
{
	uint8_t * buffer = NULL;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Sending discovery to %u.%u", neighbour->u8[0], neighbour->u8[1]);

	/* If we are currently transmitting or waiting for an ACK, do nothing */
	if( convergence_layer_transmitting ) {
		return -1;
	}

	/* Get our buffer */
	buffer = dtn_network_get_buffer();
	if( buffer == NULL ) {
		return -1;
	}

	/* Discovery Prefix */
	buffer[0] = CONVERGENCE_LAYER_TYPE_DISCOVERY;

	/* Copy the discovery message and set the length */
	memcpy(buffer + 1, payload, length);

	/* Now we are transmitting */
	convergence_layer_transmitting = 1;

	/* Send it out via the MAC */
	dtn_network_send(neighbour, length + 1, NULL);

	return 1;
}

int convergence_layer_send_ack(rimeaddr_t * destination, uint8_t sequence_number, uint8_t type, struct transmit_ticket_t * ticket)
{
	uint8_t * buffer = NULL;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Sending ACK or NACK to %u.%u for SeqNo %u with ticket %p", destination->u8[0], destination->u8[1], sequence_number, ticket);

	/* If we are currently transmitting or waiting for an ACK, do nothing */
	if( convergence_layer_transmitting ) {
		/* This ticket has to be processed ASAP, so set timestamp to 0 */
		ticket->timestamp = 0;

		return -1;
	}

	/* Get our buffer */
	buffer = dtn_network_get_buffer();
	if( buffer == NULL ) {
		return -1;
	}

	if( type == CONVERGENCE_LAYER_TYPE_ACK ) {
		// Construct the ACK
		buffer[0] = 0;
		buffer[0] |= CONVERGENCE_LAYER_TYPE_ACK & CONVERGENCE_LAYER_MASK_TYPE;
		buffer[0] |= (sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
	} else if( type == CONVERGENCE_LAYER_TYPE_NACK ) {
		// Construct the NACK
		buffer[0] = 0;
		buffer[0] |= CONVERGENCE_LAYER_TYPE_NACK & CONVERGENCE_LAYER_MASK_TYPE;
		buffer[0] |= (sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
	} else if( type == CONVERGENCE_LAYER_TYPE_TEMP_NACK ) {
		// Construct the temporary NACK
		buffer[0] = 0;
		buffer[0] |= CONVERGENCE_LAYER_TYPE_NACK & CONVERGENCE_LAYER_MASK_TYPE;
		buffer[0] |= (sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
		buffer[0] |= (CONVERGENCE_LAYER_FLAGS_FIRST) & CONVERGENCE_LAYER_MASK_FLAGS; // This flag indicates a temporary nack
	}

	/* Note down our latest attempt */
	ticket->timestamp = clock_time();

	/* Now we are transmitting */
	convergence_layer_transmitting = 1;

	/* Send it out via the MAC */
	dtn_network_send(destination, 1, ticket);

	return 1;
}

int convergence_layer_create_send_ack(rimeaddr_t * destination, uint8_t sequence_number, uint8_t type)
{
	struct transmit_ticket_t * ticket = NULL;

	/* We have to keep track of the outgoing packet, because we have to be able to retransmit */
	ticket = convergence_layer_get_transmit_ticket_priority(CONVERGENCE_LAYER_PRIORITY_HIGH);
	if( ticket == NULL ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Unable to allocate ticket to potentially retransmit ACK/NACK");
	} else {
		rimeaddr_copy(&ticket->neighbour, destination);
		ticket->sequence_number = sequence_number;
		ticket->flags |= CONVERGENCE_LAYER_QUEUE_IN_TRANSIT;

		if( type == CONVERGENCE_LAYER_TYPE_ACK ) {
			ticket->flags |= CONVERGENCE_LAYER_QUEUE_ACK;
		} else if( type == CONVERGENCE_LAYER_TYPE_NACK ) {
			ticket->flags |= CONVERGENCE_LAYER_QUEUE_NACK;
		} else if( type == CONVERGENCE_LAYER_TYPE_TEMP_NACK) {
			ticket->flags |= CONVERGENCE_LAYER_QUEUE_TEMP_NACK;
		}
	}

	return convergence_layer_send_ack(destination, sequence_number, type, ticket);
}

int convergence_layer_resend_ack(struct transmit_ticket_t * ticket)
{
	/* Check if we really have an ACK/NACK that is currently not beeing transmitted */
	if( (!(ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK) && !(ticket->flags & CONVERGENCE_LAYER_QUEUE_NACK) && !(ticket->flags & CONVERGENCE_LAYER_QUEUE_TEMP_NACK)) || (ticket->flags & CONVERGENCE_LAYER_QUEUE_IN_TRANSIT) ) {
		return 0;
	}

	/* Check for the retransmission timer */
	if( (clock_time() - ticket->timestamp) < (CONVERGENCE_LAYER_RETRANSMIT_TIMEOUT * CLOCK_SECOND) ) {
		return 0;
	}

	ticket->flags |= CONVERGENCE_LAYER_QUEUE_IN_TRANSIT;
	uint8_t type = 0;
	if( ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK ) {
		type = CONVERGENCE_LAYER_TYPE_ACK;
	} else if( ticket->flags & CONVERGENCE_LAYER_QUEUE_NACK ) {
		type = CONVERGENCE_LAYER_TYPE_NACK;
	} else if( ticket->flags & CONVERGENCE_LAYER_QUEUE_TEMP_NACK ) {
		type = CONVERGENCE_LAYER_TYPE_TEMP_NACK;
	} else {
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Unknown control packet type");
		return 0;
	}

	convergence_layer_send_ack(&ticket->neighbour, ticket->sequence_number, type, ticket);

	return 1;
}

/**
 * Return values:
 *  1 = SUCCESS
 * -1 = Temporary error
 * -2 = Permanent error
 */
int convergence_layer_parse_dataframe(rimeaddr_t * source, uint8_t * payload, uint8_t length, uint8_t flags, uint8_t sequence_number, packetbuf_attr_t rssi)
{
	struct mmem * bundlemem = NULL;
	struct bundle_t * bundle = NULL;
	int n;

	if( flags != (CONVERGENCE_LAYER_FLAGS_FIRST | CONVERGENCE_LAYER_FLAGS_LAST ) ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Bundle received %p from %u.%u with invalid flags %02X", payload, source->u8[0], source->u8[1], flags);

		/* We will never be able to parse that bundle, signal a permanent error */
		return -2;
	}

	/* Allocate memory, parse the bundle and set reference counter to 1 */
	bundlemem = bundle_recover_bundle(payload, length);
	if( !bundlemem ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Error recovering bundle");

		/* Possible not enough memory -> temporary error */
		return -1;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( !bundle ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Invalid bundle pointer");
		bundle_decrement(bundlemem);

		/* Possible not enough memory -> temporary error */
		return -1;
	}

	/* Mark the bundle as "internal" */
	bundle->source_process = &agent_process;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Bundle received from %u.%u with SeqNo %u", source->u8[0], source->u8[1], sequence_number);

	/* Store the node from which we received the bundle */
	rimeaddr_copy(&bundle->msrc, source);

	/* Store the RSSI for this packet */
	bundle->rssi = rssi;

	/* Hand over the bundle to dispatching */
	n = dispatching_dispatch_bundle(bundlemem);
	bundlemem = NULL;

	if( n ) {
		/* Dispatching was successfull! */
		return 1;
	}

	/* Temporary error */
	return -1;
}

int convergence_layer_parse_ackframe(rimeaddr_t * source, uint8_t * payload, uint8_t length, uint8_t sequence_number, uint8_t type)
{
	struct transmit_ticket_t * ticket = NULL;
	struct bundle_t * bundle = NULL;

	/* This neighbour is now unblocked */
	convergence_layer_set_unblocked(source);

	if( convergence_layer_pending == 0 ) {
		/* Poll the process to initiate transmission of the next bundle */
		process_poll(&convergence_layer_process);
	}

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming ACK from %u.%u for SeqNo %u", source->u8[0], source->u8[1], sequence_number);

	for(ticket = list_head(transmission_ticket_list);
		ticket != NULL;
		ticket = list_item_next(ticket) ) {
		if( rimeaddr_cmp(source, &ticket->neighbour) && (ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK_PEND) ) {
			break;
		}
	}

	/* Unable to find that bundle */
	if( ticket == NULL ) {
		return -1;
	}

	/* Does the originator need forward notification? */
	if( type == CONVERGENCE_LAYER_TYPE_ACK && ticket->bundle != NULL ) {
		bundle = (struct bundle_t *) MMEM_PTR(ticket->bundle);

		/* Is the forward report flag set? */
		if( bundle->flags & BUNDLE_FLAG_REP_FWD ) {
			STATUSREPORT.send(ticket->bundle, NODE_FORWARDED_BUNDLE, NO_ADDITIONAL_INFORMATION);
		}
	}

	/* TODO: Handle temporary NACKs separately here */
	if( type == CONVERGENCE_LAYER_TYPE_ACK ) {
		/* Bundle has been ACKed and is now done */
		ticket->flags = CONVERGENCE_LAYER_QUEUE_DONE;

		/* Notify routing module */
		ROUTING.sent(ticket, ROUTING_STATUS_OK);
	} else if( type == CONVERGENCE_LAYER_TYPE_NACK ) {
		/* Bundle has been NACKed and is now done */
		ticket->flags = CONVERGENCE_LAYER_QUEUE_FAIL;

		/* Notify routing module */
		ROUTING.sent(ticket, ROUTING_STATUS_NACK);
	}

	/* We can free the bundle memory */
	if( ticket->bundle != NULL ) {
		bundle_decrement(ticket->bundle);
		ticket->bundle = NULL;
	}

	return 1;
}

int convergence_layer_incoming_frame(rimeaddr_t * source, uint8_t * payload, uint8_t length, packetbuf_attr_t rssi)
{
	uint8_t * data_pointer = NULL;
	uint8_t data_length = 0;
	uint8_t header;
	int ret = 0;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming frame from %u.%u", source->u8[0], source->u8[1]);

	/* Notify the discovery module, that we have seen a peer */
	DISCOVERY.alive(source);

	/* Check the COMPAT information */
	if( (payload[0] & CONVERGENCE_LAYER_MASK_COMPAT) != CONVERGENCE_LAYER_COMPAT ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_INF, "Ignoring incoming frame from %u.%u", source->u8[0], source->u8[1]);
		return -1;
	}

	header = payload[0];
	data_pointer = payload + 1;
	data_length = length - 1;

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_DATA ) {
		/* is data */
		int flags = 0;
		int sequence_number = 0;

		flags = (header & CONVERGENCE_LAYER_MASK_FLAGS) >> 0;
		sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming data frame from %u.%u with SeqNo %u", source->u8[0], source->u8[1], sequence_number);

		/* Parse the incoming data frame */
		ret = convergence_layer_parse_dataframe(source, data_pointer, data_length, flags, sequence_number, rssi);

		if( ret >= 0 ) {
			/* Send ACK */
			convergence_layer_create_send_ack(source, sequence_number + 1, CONVERGENCE_LAYER_TYPE_ACK);
		} else if( ret == -1 ) {
			/* Send temporary NACK */
			convergence_layer_create_send_ack(source, sequence_number + 1, CONVERGENCE_LAYER_TYPE_TEMP_NACK);
		} else if( ret == -2 ) {
			/* Send permanent NACK */
			convergence_layer_create_send_ack(source, sequence_number + 1, CONVERGENCE_LAYER_TYPE_NACK);
		} else {
			/* FAIL */
		}

		return 1;
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_DISCOVERY ) {
		/* is discovery */
		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming discovery frame from %u.%u", source->u8[0], source->u8[1]);

		DISCOVERY.receive(source, data_pointer, data_length);

		return 1;
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_ACK ) {
		/* is ACK */
		int sequence_number = 0;

		sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming Ack frame from %u.%u with SeqNo %u", source->u8[0], source->u8[1], sequence_number);

		convergence_layer_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_ACK);

		return 1;
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_NACK ) {
		/* is NACK */
		int sequence_number = 0;

		sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming Nack frame from %u.%u with SeqNo %u", source->u8[0], source->u8[1], sequence_number);

		convergence_layer_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_NACK);

		return 1;
	}

	return 0;
}

int convergence_layer_status(void * pointer, uint8_t outcome)
{
	struct transmit_ticket_t * ticket = NULL;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "MAC callback for %p with %d", pointer, outcome);

	/* Something has been sent, so the radio is not blocked anymore */
	convergence_layer_transmitting = 0;

	/* Notify the process to commence transmitting outgoing bundles */
	if( convergence_layer_pending == 0 ) {
		if( outcome == CONVERGENCE_LAYER_STATUS_NOSEND ) {
			/* Send event to slow the stuff down */
			etimer_set(&convergence_layer_backoff, 0.5 * CLOCK_SECOND);
			convergence_layer_backoff_pending = 1;
		} else {
			/* Poll to make it faster */
			process_poll(&convergence_layer_process);
		}

		convergence_layer_pending = 1;
	}

	if( pointer == NULL ) {
		/* Must be a discovery message */
		return 0;
	}

	ticket = (struct transmit_ticket_t *) pointer;

	if( (ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK) || (ticket->flags & CONVERGENCE_LAYER_QUEUE_NACK) || (ticket->flags & CONVERGENCE_LAYER_QUEUE_TEMP_NACK) ) {
		/* Unset IN_TRANSIT flag */
		ticket->flags &= ~CONVERGENCE_LAYER_QUEUE_IN_TRANSIT;

		/* Must be a NACK or ACK */
		if( outcome == CONVERGENCE_LAYER_STATUS_OK ) {
			/* Great! */
			convergence_layer_free_transmit_ticket(ticket);

			return 1;
		}

		/* Fatal error, retry is pointless */
		if( outcome == CONVERGENCE_LAYER_STATUS_FATAL ) {
			convergence_layer_free_transmit_ticket(ticket);
			return 1;
		}

		if( outcome == CONVERGENCE_LAYER_STATUS_NOSEND ) {
			// Has not been transmitted, so retry right away
			ticket->timestamp = 0;
			ticket->failed_tries ++;
		}

		/* Increase the retry counter */
		ticket->tries ++;

		/* Give up on too many retries */
		if( ticket->tries >= CONVERGENCE_LAYER_RETRANSMIT_TRIES || ticket->failed_tries >= CONVERGENCE_LAYER_FAILED_RETRIES) {
			LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "CL: Giving up on ticket %p after %d (or %d) tries", ticket, ticket->tries, ticket->failed_tries);
			convergence_layer_free_transmit_ticket(ticket);

			return 0;
		}

		return 1;
	}

	/* Bundle was transmitted successfully */
	if( outcome == CONVERGENCE_LAYER_STATUS_OK ) {
		// Bundle is sent, now waiting for the app-layer ACK
		ticket->flags = CONVERGENCE_LAYER_QUEUE_ACTIVE | CONVERGENCE_LAYER_QUEUE_ACK_PEND;

		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "LL Ack received, waiting for App-layer ACK with SeqNo %u", ticket->sequence_number);

		/* It is unlikely that we have to retransmit this bundle, so free up memory */
		if( ticket->bundle != NULL ) {
			bundle_decrement(ticket->bundle);
			ticket->bundle = NULL;
		}

		return 1;
	}

	/* Fatal error, no retry necessary */
	if( outcome == CONVERGENCE_LAYER_STATUS_FATAL ) {
		/* This neighbour is now unblocked */
		convergence_layer_set_unblocked(&ticket->neighbour);

		/* Notify routing module */
		ROUTING.sent(ticket, ROUTING_STATUS_ERROR);

		return 1;
	}

	/* Something went wrong, unblock the neighbour */
	convergence_layer_set_unblocked(&ticket->neighbour);

	/* Bundle did not get an ACK, increase try counter */
	if( outcome == CONVERGENCE_LAYER_STATUS_NOACK ) {
		ticket->tries ++;
	} else if( outcome == CONVERGENCE_LAYER_STATUS_NOSEND ) {
		ticket->failed_tries ++;
	}

	if( ticket->tries >= CONVERGENCE_LAYER_RETRIES || ticket->failed_tries >= CONVERGENCE_LAYER_FAILED_RETRIES ) {
		/* Bundle fails over and over again, notify routing */
		ticket->flags = CONVERGENCE_LAYER_QUEUE_FAIL;

		/* Notify routing module */
		ROUTING.sent(ticket, ROUTING_STATUS_FAIL);

		/* We can already free the bundle memory */
		if( ticket->bundle != NULL ) {
			bundle_decrement(ticket->bundle);
			ticket->bundle = NULL;
		}

		return 1;
	}

	/* Transmission did not work, retry it right away */
	ticket->flags = CONVERGENCE_LAYER_QUEUE_ACTIVE;

	return 1;
}

int convergence_layer_delete_bundle(uint32_t bundle_number)
{
	struct transmit_ticket_t * ticket = NULL;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Deleting tickets for bundle %lu", bundle_number);

	for(ticket = list_head(transmission_ticket_list);
		ticket != NULL;
		ticket = list_item_next(ticket) ) {
		if( ticket->bundle_number == bundle_number ) {
			break;
		}
	}

	/* Unable to find that bundle */
	if( ticket == NULL ) {
		return -1;
	}

	/* free the ticket's memory */
	convergence_layer_free_transmit_ticket(ticket);

	return 1;
}

int convergence_layer_is_blocked(rimeaddr_t * neighbour)
{
	struct blocked_neighbour_t * n = NULL;

	for( n = list_head(blocked_neighbour_list);
		 n != NULL;
		 n = list_item_next(n) ) {
		if( rimeaddr_cmp(neighbour, &n->neighbour) ) {
			return 1;
		}
	}

	return 0;
}

int convergence_layer_set_blocked(rimeaddr_t * neighbour)
{
	struct blocked_neighbour_t * n = NULL;

	n = memb_alloc(&blocked_neighbour_mem);
	if( n == NULL ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Cannot allocate neighbour memory");
		return -1;
	}

	/* Fill the struct */
	rimeaddr_copy(&n->neighbour, neighbour);
	n->timestamp = clock_time();

	/* Add it to the list */
	list_add(blocked_neighbour_list, n);

	return 1;
}

int convergence_layer_set_unblocked(rimeaddr_t * neighbour)
{
	struct blocked_neighbour_t * n = NULL;

	for( n = list_head(blocked_neighbour_list);
		 n != NULL;
		 n = list_item_next(n) ) {
		if( rimeaddr_cmp(neighbour, &n->neighbour) ) {
			list_remove(blocked_neighbour_list, n);
			memb_free(&blocked_neighbour_mem, n);
			return 1;
		}
	}

	return 0;
}

void check_blocked_neighbours() {
	struct blocked_neighbour_t * n = NULL;
	struct transmit_ticket_t * ticket = NULL;

	for( n = list_head(blocked_neighbour_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		if( (clock_time() - n->timestamp) >= (((clock_time_t) CLOCK_SECOND) * ((clock_time_t) CONVERGENCE_LAYER_TIMEOUT) ) ) {
			/* We have a neighbour that takes quite long to reply apparently -
			 * unblock him and resend the pending bundle
			 */
			break;
		}
	}

	/* Nothing to do for us */
	if( n == NULL ) {
		return;
	}

	/* Go and find the currently transmitting ticket to that neighbour */
	for( ticket = list_head(transmission_ticket_list);
		 ticket != NULL;
		 ticket = list_item_next(ticket) ) {
		if( rimeaddr_cmp(&ticket->neighbour, &n->neighbour) && (ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK_PEND) ) {
			break;
		}
	}

	/* Unblock the neighbour */
	convergence_layer_set_unblocked(&n->neighbour);

	LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Neighbour %u.%u stale, removing lock", n->neighbour.u8[0], n->neighbour.u8[1]);

	/* There seems to be no ticket, nothing to do for us */
	if( ticket == NULL ) {
		return;
	}

	/* Otherwise: just reactivate the ticket, it will be transmitted again */
	ticket->flags = CONVERGENCE_LAYER_QUEUE_ACTIVE;

	if( convergence_layer_pending == 0 ) {
		/* Tell the process to resend the bundles */
		process_poll(&convergence_layer_process);
	}
}

int convergence_layer_neighbour_down(rimeaddr_t * neighbour) {
	struct transmit_ticket_t * ticket = NULL;
	int changed = 1;

	if( neighbour == NULL ) {
		return -1;
	}

	while( changed ) {
		changed = 0;

		/* Go and look for a ticket for this neighbour */
		for(ticket = list_head(transmission_ticket_list);
			ticket != NULL;
			ticket = list_item_next(ticket) ) {

			if( rimeaddr_cmp(neighbour, &ticket->neighbour) ) {
				/* Notify routing module */
				ROUTING.sent(ticket, ROUTING_STATUS_FAIL);

				/* Mark as changed */
				changed = 1;

				/* Stop look and start over again */
				break;
			}
		}
	}

	/* Remove potentially stale lock for neighbour */
	convergence_layer_set_unblocked(neighbour);

	return 1;
}

void convergence_layer_show_tickets() {
	struct transmit_ticket_t * ticket = NULL;

	printf("--- TICKET LIST\n");
	for(ticket = list_head(transmission_ticket_list);
		ticket != NULL;
		ticket = list_item_next(ticket) ) {

		printf("B %10lu to %02u.%02u with SeqNo %u and Flags %02X\n", ticket->bundle_number, ticket->neighbour.u8[0], ticket->neighbour.u8[1], ticket->sequence_number, ticket->flags);
	}

	printf("---\n");
}

PROCESS_THREAD(convergence_layer_process, ev, data)
{
	struct transmit_ticket_t * ticket = NULL;
	static struct etimer stale_timer;
	int n;

	PROCESS_BEGIN();

	/* Initialize ticket storage */
	memb_init(&transmission_ticket_mem);
	list_init(transmission_ticket_list);

	/* Initialize neighbour storage */
	memb_init(&blocked_neighbour_mem);
	list_init(blocked_neighbour_list);

	LOG(LOGD_DTN, LOG_CL, LOGL_INF, "CL process is running");

	/* Initialize state */
	convergence_layer_transmitting = 0;
	outgoing_sequence_number = 0;
	convergence_layer_queue = 0;
	convergence_layer_pending = 0;

	/* Set timer */
	etimer_set(&stale_timer, CLOCK_SECOND);

	while(1) {
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_CONTINUE || etimer_expired(&stale_timer) || ev == PROCESS_EVENT_TIMER);

		if( etimer_expired(&stale_timer) ) {
			check_blocked_neighbours();
			etimer_restart(&stale_timer);
		}

		if( ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_CONTINUE || (convergence_layer_backoff_pending && etimer_expired(&convergence_layer_backoff)) ) {
			convergence_layer_pending = 0;

			/* Stop timer to avoid it firing again */
			if ( (convergence_layer_backoff_pending && etimer_expired(&convergence_layer_backoff)) ) {
				etimer_stop(&convergence_layer_backoff);
				convergence_layer_backoff_pending = 0;
			}

			/* If we are currently transmitting, we cannot send another bundle */
			if( convergence_layer_transmitting ) {
				/* We will get polled again when the MAC layers calls us back,
				 * so lean back and relax
				 */
				continue;
			}


			/* If we have been woken up, it must have been a poll to transmit outgoing bundles */
			for(ticket = list_head(transmission_ticket_list);
				ticket != NULL;
				ticket = list_item_next(ticket) ) {
				if( ((ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK) || (ticket->flags & CONVERGENCE_LAYER_QUEUE_NACK)) && !(ticket->flags & CONVERGENCE_LAYER_QUEUE_IN_TRANSIT) ) {
					n = convergence_layer_resend_ack(ticket);
					if( n ) {
						/* Transmission happened */
						break;
					}
				}

				/* Tickets that are in any other state than ACTIVE cannot be transmitted */
				if( ticket->flags != CONVERGENCE_LAYER_QUEUE_ACTIVE ) {
					continue;
				}

				/* Neighbour for which we are currently waiting on app-layer ACKs cannot receive anything now */
				if( convergence_layer_is_blocked(&ticket->neighbour) ) {
					continue;
				}

				/* Send the bundle just now */
				convergence_layer_send_bundle(ticket);

				/* Radio is busy now, defer */
				break;
			}
		}
	}
	PROCESS_END();
}
