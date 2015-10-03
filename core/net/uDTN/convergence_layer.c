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
#include "convergence_layer.h"

#include <stdbool.h>
#include <string.h> // memset

#include "FreeRTOS.h"
#include "task.h"

#include "net/packetbuf.h"
#include "net/netstack.h"
#include "net/linkaddr.h"
#include "lib/list.h"
#include "lib/logging.h"

#include "agent.h"
#include "storage.h"
#include "discovery.h"
#include "dtn_network.h"
#include "dispatching.h"
#include "bundleslot.h"
#include "statusreport.h"
#include "bundle_ageing.h"
#include "convergence_layer_udp_dgram.h"



/**
 * Structure to keep track of neighbours from which we are currently awaiting app-layer ACKs
 */
struct blocked_neighbour_t {
	struct blocked_neighbour_t * next;

	/* Address of the neighbour */
	cl_addr_t neighbour;

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
int convergence_layer_is_blocked(const cl_addr_t * const neighbour);
int convergence_layer_set_blocked(const cl_addr_t* const neighbour);
int convergence_layer_set_unblocked(const cl_addr_t * const neighbour);

/**
 * CL process
 */
static TaskHandle_t convergence_layer_task;
static void convergence_layer_process(void* p);

/**
 * MUTEX to avoid flooding the MAC layer with outgoing bundles
 */
static int convergence_layer_transmitting = 0;

/**
 * Keep track of the allocated tickets
 */
int convergence_layer_slots;

/**
 * Keep track of the tickets in queue
 */
static int convergence_layer_queue = 0;

/**
 * Use a unique sequence number of each outgoing segment
 */
static uint8_t outgoing_sequence_number = 0;

/**
 * Indicate when a continue event or poll to ourselves is pending to avoid
 * exceeding the event queue size 
 */
static uint8_t convergence_layer_pending = 0;

/**
 * Backoff timer
 */
static volatile bool convergence_layer_backoff_pending = false;

void convergence_layer_show_tickets();

bool convergence_layer_init(void)
{
	// Start CL process
	if ( !xTaskCreate(convergence_layer_process, "CL process", 0x200, NULL, 1, &convergence_layer_task) ) {
		return false;
	}

	return true;
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
	ticket->timestamp = xTaskGetTickCount();

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
	/* Remove our reference to the bundle */
	if( ticket->bundle != NULL ) {
		bundle_decrement(ticket->bundle);
		ticket->bundle = NULL;
	}

	/* Also free MMEM if it was allocate to serialize the bundle */
	if( ticket->buffer.ptr != NULL ) {
		mmem_free(&ticket->buffer);
		ticket->buffer.ptr = NULL;
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

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(&ticket->neighbour, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Enqueuing bundle %lu to %s, queue is at %u entries", ticket->bundle_number, addr_str, convergence_layer_queue);

	/* The ticket is now active a ready for transmission */
	ticket->flags |= CONVERGENCE_LAYER_QUEUE_ACTIVE;

	if( convergence_layer_pending == 0 ) {
		/* Poll the process to initiate transmission */
		xTaskNotify(convergence_layer_task, 0, eNoAction);
	}

	convergence_layer_queue++;

	return 1;
}


static int convergence_layer_dgram_encode_bundle(struct transmit_ticket_t* const ticket)
{
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Encoding bundle %lu", ticket->bundle_number);

	/* Now allocate a buffer to serialize the bundle
	 * The size is a rough estimation here and will be reallocated later on */
	if(mmem_alloc(&ticket->buffer, ticket->bundle->size) < 1) {
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Bundle %lu could not be encoded, not enough memory for %u bytes", ticket->bundle_number, ticket->bundle->size);
		return -1;
	}

	/* Encode the bundle into our temporary buffer */
	const size_t length = bundle_encode_bundle(ticket->bundle, (uint8_t *) MMEM_PTR(&ticket->buffer), ticket->buffer.size);

	if( length < 0 ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Bundle %lu could not be encoded, error occured", ticket->bundle_number);
		mmem_free(&ticket->buffer);
		ticket->buffer.ptr = NULL;
		return -1;
	}

	/* Decrease memory size to what is actually needed */
	if(mmem_realloc(&ticket->buffer, length) < 1) {
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Bundle %lu could not be encoded, realloc failed", ticket->bundle_number);
		mmem_free(&ticket->buffer);
		ticket->buffer.ptr = NULL;
		return -1;
	}

	/* Initialize the state for this bundle */
	ticket->offset_sent = 0;
	ticket->offset_acked = 0;

	return 0;
}


int convergence_layer_lowpan_dgram_send_bundle(const cl_addr_t* const dest, const int sequence_number, const uint8_t flags,
											const uint8_t* const payload, const size_t length, const void* const reference)
{
	/* Get the outgoing network buffer */
	uint8_t* const buffer = dtn_network_get_buffer();
	if( buffer == NULL ) {
		return -1;
	}

	/* add one byte for the dgram:lowpan header */
	const uint8_t length_to_send = length + 1;

	/* fail if the buffer is not big enough */
	if (length_to_send > dtn_network_get_buffer_length()) {
		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(dest, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "DTN network buffer to small for bundle (seq %u, flags %x, len %lu) to  addr %s",
			sequence_number, flags, length_to_send, addr_str);
		return -2;
	}

	/* Initialize the header field */
	buffer[0] = CONVERGENCE_LAYER_TYPE_DATA & CONVERGENCE_LAYER_MASK_TYPE;

	/* Put the sequence number for this bundle into the outgoing header */
	buffer[0] |= (sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
	buffer[0] |= flags & CONVERGENCE_LAYER_MASK_FLAGS;

	/* copy the payload */
	memcpy(&buffer[1], payload, length);

	/* And send it out */
	// TODO remove const cast
	dtn_network_send((linkaddr_t*)&dest->lowpan, length_to_send, (void*)reference);

	return 1;
}


static int convergence_layer_dgram_send_bundle(struct transmit_ticket_t* const ticket, const uint8_t flags,
											   const uint8_t* const payload, const size_t length)
{
	/* Flag the bundle as being in transit now */
	ticket->flags |= CONVERGENCE_LAYER_QUEUE_IN_TRANSIT;

	/* Now we are transmitting */
	convergence_layer_transmitting = 1;

	/* This neighbour is blocked, until we have received the App Layer ACK or NACK */
	convergence_layer_set_blocked(&ticket->neighbour);

	int ret = -1;
	if (ticket->neighbour.isIP) {
		ret = convergence_layer_udp_dgram_send_bundle(&ticket->neighbour, ticket->sequence_number, flags, payload, length, ticket);
	} else {
		ret = convergence_layer_lowpan_dgram_send_bundle(&ticket->neighbour, ticket->sequence_number, flags, payload, length, ticket);
	}

	if (ret < 0) {
		bundle_decrement(ticket->bundle);
		ticket->bundle = NULL;
	}

	return ret;
}


static int convergence_layer_dgram_prepare_segmentation(struct transmit_ticket_t * ticket)
{
	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(&ticket->neighbour, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Sending bundle %lu to %s with ticket %p (flags 0x%x)",
		ticket->bundle_number, addr_str, ticket, ticket->flags);

	if( !(ticket->flags & CONVERGENCE_LAYER_QUEUE_MULTIPART) ) {
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
		const struct bundle_t* const bundle = (struct bundle_t *) MMEM_PTR(ticket->bundle);
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
	}


	// TODO only enncode bundle ones, if it is a multipart bundle
	// otherwise sending will always start at the beginning of the package
	if (convergence_layer_dgram_encode_bundle(ticket) < 0) {
		ticket->flags &= ~CONVERGENCE_LAYER_QUEUE_MULTIPART;
		return -1;
	}

	/* We have to use a heuristic to estimate if the bundle will be a multipart bundle */
	if( ticket->buffer.size > CONVERGENCE_LAYER_MAX_LENGTH && !(ticket->flags & CONVERGENCE_LAYER_QUEUE_MULTIPART) ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Try to send bundle %lu as mutlipart bundle (buf %p, size %lu, flags 0x%x)",
			ticket->bundle_number, ticket->buffer, ticket->buffer.size, ticket->flags);

		/*
		 * We do not need the original bundle anymore
		 * It is already encoded to ticket->buffer.
		 * The encoding was done by convergence_layer_dgram_encode_bundle().
		 * The ticket can only be deleted,
		 * if the first part of the bundle was send.
		 * If sending of the first part,
		 * the ageing check has to be repeated.
		 */
		bundle_decrement(ticket->bundle);
		ticket->bundle = NULL;

		/* This is a bundle for multiple segments and we have our first look at it */
		ticket->flags |= CONVERGENCE_LAYER_QUEUE_MULTIPART;

		/* Initialize the state for this bundle */
		ticket->sequence_number = outgoing_sequence_number;

		/* Calculate the number of segments we will need */
		const size_t segments = ( ticket->buffer.size + (CONVERGENCE_LAYER_MAX_LENGTH / 2) ) / CONVERGENCE_LAYER_MAX_LENGTH;

		/* And reserve the sequence number space for this bundle to allow for consequtive numbers */
		outgoing_sequence_number = (outgoing_sequence_number + segments) % 4;
	}

	/* Check if this is a multipart bundle */
	if( ticket->flags & CONVERGENCE_LAYER_QUEUE_MULTIPART ) {
		/* Calculate the remaining length */
		const size_t length = ticket->buffer.size - ticket->offset_acked;

		/* Is it possible, that we send a single-part bundle here because the heuristic
		 * from above failed. So be it.
		 */
		uint8_t flags = CONVERGENCE_LAYER_FLAGS_FIRST | CONVERGENCE_LAYER_FLAGS_LAST;
		if( length <= CONVERGENCE_LAYER_MAX_LENGTH && ticket->offset_acked == 0 ) {
			/* One bundle per segment, standard flags */
			flags = CONVERGENCE_LAYER_FLAGS_FIRST | CONVERGENCE_LAYER_FLAGS_LAST;
		} else if( ticket->offset_acked == 0 ) {
			/* First segment of a bundle */
			flags = CONVERGENCE_LAYER_FLAGS_FIRST;
		} else if( length <= CONVERGENCE_LAYER_MAX_LENGTH ) {
			/* Last segment of a bundle */
			flags = CONVERGENCE_LAYER_FLAGS_LAST;
		} else if( length > CONVERGENCE_LAYER_MAX_LENGTH) {
			/* A segment in the middle of a bundle */
			flags = 0;
		}

		/* one byte for the CL header */
		const size_t length_to_sent = (length > (CONVERGENCE_LAYER_MAX_LENGTH - 1)) ? (CONVERGENCE_LAYER_MAX_LENGTH - 1) : length;

		const uint8_t* const buffer = ((uint8_t*) MMEM_PTR(&ticket->buffer)) + ticket->offset_acked;
		const int ret = convergence_layer_dgram_send_bundle(ticket, flags, buffer, length_to_sent);

		/* Every segment so far has been acked */
		if( ticket->offset_sent == ticket->offset_acked ) {
			/* It is the first time that we are sending this segment */
			ticket->offset_sent += length_to_sent;

			/* Increment the sequence number for the new segment, except for the first segment */
			if( ticket->offset_sent != 0 ) {
				ticket->sequence_number = (ticket->sequence_number + 1) % 4;
			}
		}

		return ret;
	} else {

		/* Initialize the sequence number */
		ticket->sequence_number = outgoing_sequence_number;
		outgoing_sequence_number = (outgoing_sequence_number + 1) % 4;

		/* One bundle per segment, standard flags */
		const uint8_t flags = CONVERGENCE_LAYER_FLAGS_FIRST | CONVERGENCE_LAYER_FLAGS_LAST;
		const uint8_t* const buffer = ((uint8_t*) MMEM_PTR(&ticket->buffer));
		return convergence_layer_dgram_send_bundle(ticket, flags, buffer, ticket->buffer.size);
	}
}


int convergence_layer_send_discovery(uint8_t * payload, uint8_t length, linkaddr_t * neighbour)
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


int convergence_layer_lowpan_dgram_send_ack(const cl_addr_t* const destination, const int sequence_number, const int type,
											 const void* const reference)
{
	/* Get our buffer */
	uint8_t* const buffer = dtn_network_get_buffer();
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

	/* Send it out via the MAC */
	dtn_network_send((linkaddr_t*)&destination->lowpan, 1, (void*)reference);

	return 1;
}


static int convergence_layer_send_ack(const cl_addr_t* const destination, const uint8_t sequence_number, const uint8_t type,
							   struct transmit_ticket_t* const ticket)
{
	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(destination, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Sending ACK or NACK to %s for SeqNo %u with ticket %p", addr_str, sequence_number, ticket);

	/* If we are currently transmitting or waiting for an ACK, do nothing */
	if( convergence_layer_transmitting ) {
		/* This ticket has to be processed ASAP, so set timestamp to 0 */
		ticket->timestamp = 0;

		return -1;
	}

	/* Note down our latest attempt */
	ticket->timestamp = xTaskGetTickCount();

	/* Now we are transmitting */
	convergence_layer_transmitting = 1;

	if (destination->isIP) {
		return convergence_layer_udp_dgram_send_ack(destination, sequence_number, type, ticket);
	} else {
		return convergence_layer_lowpan_dgram_send_ack(destination, sequence_number, type, ticket);
	}
}

static int convergence_layer_create_send_ack(const cl_addr_t* const destination, const uint8_t sequence_number, const uint8_t type)
{
	struct transmit_ticket_t * ticket = NULL;

	/* We have to keep track of the outgoing packet, because we have to be able to retransmit */
	ticket = convergence_layer_get_transmit_ticket_priority(CONVERGENCE_LAYER_PRIORITY_HIGH);
	if( ticket == NULL ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Unable to allocate ticket to potentially retransmit ACK/NACK");
	} else {
		cl_addr_copy(&ticket->neighbour, destination);
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
	if( (xTaskGetTickCount() - ticket->timestamp) < pdMS_TO_TICKS(CONVERGENCE_LAYER_RETRANSMIT_TIMEOUT) ) {
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
static int convergence_layer_parse_dataframe(const cl_addr_t* const source, const uint8_t* payload, const uint8_t payload_length,
											 const uint8_t flags, const uint8_t sequence_number, const packetbuf_attr_t rssi)
{
	struct mmem * bundlemem = NULL;
	struct bundle_t * bundle = NULL;
	struct transmit_ticket_t * ticket = NULL;
	int n;
	int length;
	int ret;

	/* Note down the payload length */
	length = payload_length;

	if( flags != (CONVERGENCE_LAYER_FLAGS_FIRST | CONVERGENCE_LAYER_FLAGS_LAST ) ) {
		/* We have a multipart bundle here */
		if( flags == CONVERGENCE_LAYER_FLAGS_FIRST ) {
			/* Beginning of a new bundle from a peer, remove old tickets */
			for( ticket = list_head(transmission_ticket_list);
				 ticket != NULL;
				 ticket = list_item_next(ticket) ) {
				if( cl_addr_cmp(&ticket->neighbour, source) && (ticket->flags & CONVERGENCE_LAYER_QUEUE_MULTIPART_RECV) ) {
					break;
				}
			}

			/* We found a ticket, remove it */
			if( ticket != NULL ) {
				char addr_str[CL_ADDR_STRING_LENGTH];
				cl_addr_string(source, addr_str, sizeof(addr_str));
				LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Resynced to peer %s, throwing away old buffer", addr_str);
				convergence_layer_free_transmit_ticket(ticket);
				ticket = NULL;
			}

			/* Allocate a new ticket for the incoming bundle */
			ticket = convergence_layer_get_transmit_ticket_priority(CONVERGENCE_LAYER_PRIORITY_HIGH);

			if( ticket == NULL ) {
				LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Unable to allocate multipart receive ticket");
				return -1;
			}

			/* Fill the fields of the ticket */
			cl_addr_copy(&ticket->neighbour, source);
			ticket->flags = CONVERGENCE_LAYER_QUEUE_MULTIPART_RECV;
			ticket->timestamp = xTaskGetTickCount();
			ticket->sequence_number = sequence_number;

			/* Now allocate some memory */
			ret = mmem_alloc(&ticket->buffer, length);

			if( ret < 1 ) {
				LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Unable to allocate multipart receive buffer of %u bytes", length);
				convergence_layer_free_transmit_ticket(ticket);
				ticket = NULL;
				return -1;
			}

			/* Copy the payload into the buffer */
			memcpy(MMEM_PTR(&ticket->buffer), payload, length);

			/* We are waiting for more segments, return now */
			return 1;
		} else {
			/* Either the middle of the end of a bundle, go look for the ticket */
			for( ticket = list_head(transmission_ticket_list);
				 ticket != NULL;
				 ticket = list_item_next(ticket) ) {
				if( cl_addr_cmp(&ticket->neighbour, source) && (ticket->flags & CONVERGENCE_LAYER_QUEUE_MULTIPART_RECV) ) {
					break;
				}
			}

			/* Cannot find a ticket, discard segment */
			if( ticket == NULL ) {
				char addr_str[CL_ADDR_STRING_LENGTH];
				cl_addr_string(source, addr_str, sizeof(addr_str));
				LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Segment from peer %s does not match any bundles in progress, discarding", addr_str);
				return -1;
			}

			if( sequence_number != (ticket->sequence_number + 1) % 4 ) {
				char addr_str[CL_ADDR_STRING_LENGTH];
				cl_addr_string(source, addr_str, sizeof(addr_str));
				LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Segment from peer %s is out of sequence. Recv %u, Exp %u", addr_str, sequence_number, (ticket->sequence_number + 1) % 4);
				return 1;
			}

			/* Store the last received and valid sequence number */
			ticket->sequence_number = sequence_number;

			/* Note down the old length to know where to start */
			n = ticket->buffer.size;

			/* Allocate more memory */
			ret = mmem_realloc(&ticket->buffer, ticket->buffer.size + length);

			if( ret < 1 ) {
				LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Unable to re-allocate multipart receive buffer of %u bytes", ticket->buffer.size + length);
				convergence_layer_free_transmit_ticket(ticket);
				return -1;
			}

			/* Update timestamp to avoid the ticket from timing out */
			ticket->timestamp = xTaskGetTickCount();

			/* And append the payload */
			memcpy(((uint8_t *) MMEM_PTR(&ticket->buffer)) + n, payload, length);
		}

		if( flags & CONVERGENCE_LAYER_FLAGS_LAST ) {
			/* We have the last segment, change pointer so that the rest of the function works as planned */
			payload = (uint8_t *) MMEM_PTR(&ticket->buffer);
			length = ticket->buffer.size;

			char addr_str[CL_ADDR_STRING_LENGTH];
			cl_addr_string(source, addr_str, sizeof(addr_str));
			LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "%u byte multipart bundle received from %s, parsing", length, addr_str);
		} else {
			/* We are waiting for more segments, return now */
			return 1;
		}
	}

	/* Allocate memory, parse the bundle and set reference counter to 1 */
	bundlemem = bundle_recover_bundle((uint8_t*)payload, length);

	/* We do not need the ticket anymore if there was one, deallocate it */
	if( ticket != NULL ) {
		convergence_layer_free_transmit_ticket(ticket);
		ticket = NULL;
	}

	if( !bundlemem ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Error recovering bundle");

		/* Possibly not enough memory -> temporary error */
		return -1;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( !bundle ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Invalid bundle pointer");
		bundle_decrement(bundlemem);

		/* Possibly not enough memory -> temporary error */
		return -1;
	}

	/* Check for bundle expiration */
	if( bundle_ageing_is_expired(bundlemem) ) {
		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Bundle received from %s with SeqNo %u is expired", addr_str, sequence_number);
		bundle_decrement(bundlemem);

		/* Send permanent rejection */
		return -2;
	}

	/* Mark the bundle as "internal" */
	agent_set_bundle_source(bundle);

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(source, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Bundle from ipn:%lu.%lu (to ipn:%lu.%lu) received from %s with SeqNo %u", bundle->src_node, bundle->src_srv, bundle->dst_node, bundle->dst_srv, addr_str, sequence_number);

	/* Store the node from which we received the bundle */
	cl_addr_copy(&bundle->msrc, source);

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

int convergence_layer_parse_ackframe(const cl_addr_t* const source, const uint8_t* const payload, const uint8_t length,
											const uint8_t sequence_number, const uint8_t type, const uint8_t flags)
{
	struct transmit_ticket_t * ticket = NULL;
	struct bundle_t * bundle = NULL;

	/* This neighbour is now unblocked */
	convergence_layer_set_unblocked(source);

	if( convergence_layer_pending == 0 ) {
		/* Poll the process to initiate transmission of the next bundle */
		xTaskNotify(convergence_layer_task, 0, eNoAction);
	}

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(source, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming ACK from %s for SeqNo %u", addr_str, sequence_number);

	for(ticket = list_head(transmission_ticket_list);
		ticket != NULL;
		ticket = list_item_next(ticket) ) {
		if( cl_addr_cmp(source, &ticket->neighbour) && (ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK_PEND) ) {
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
		if( ticket->flags & CONVERGENCE_LAYER_QUEUE_MULTIPART ) {
			if( sequence_number == (ticket->sequence_number + 1) % 4 ) {
				// ACK received
				ticket->offset_acked = ticket->offset_sent;

				if( ticket->offset_acked == ticket->buffer.size ) {
					/* Last segment, we are done */
					LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Last Segment of bundle %lu acked, done", ticket->bundle_number);
				} else {
					/* There are more segments, keep on sending */
					LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "One Segment of bundle %lu acked, more to come", ticket->bundle_number);
					return 1;
				}
			} else {
				/* Duplicate or out of sequence ACK, ignore it */
				LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Duplicate ACK for bundle %lu received", ticket->bundle_number);
				return 1;
			}
		}

		/* Bundle has been ACKed and is now done */
		ticket->flags = CONVERGENCE_LAYER_QUEUE_DONE;

		/* Notify routing module */
		ROUTING.sent(ticket, ROUTING_STATUS_OK);
	} else if( type == CONVERGENCE_LAYER_TYPE_NACK ) {
		/* Bundle has been NACKed and is now done */
		ticket->flags = CONVERGENCE_LAYER_QUEUE_FAIL;

		/* Notify routing module */
		if( flags & CONVERGENCE_LAYER_FLAGS_FIRST ) {
			/* Temporary NACK */
			ROUTING.sent(ticket, ROUTING_STATUS_TEMP_NACK);
		} else {
			/* Permanent NACK */
			ROUTING.sent(ticket, ROUTING_STATUS_NACK);
		}
	}

	/* We can free the bundle memory */
	if( ticket->bundle != NULL ) {
		bundle_decrement(ticket->bundle);
		ticket->bundle = NULL;
	}

	return 1;
}


int convergence_layer_incoming_data(const cl_addr_t* const source, const uint8_t* const data_pointer, const uint8_t data_length,
									const packetbuf_attr_t rssi, const int sequence_number, const int flags)
{
	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(source, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming data frame from %s with SeqNo %u and Flags %02X", addr_str, sequence_number, flags);

	/* Parse the incoming data frame */
	const int ret = convergence_layer_parse_dataframe(source, data_pointer, data_length, flags, sequence_number, rssi);

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


int convergence_layer_incoming_frame(const cl_addr_t* const source, const uint8_t* const payload, const uint8_t length, const packetbuf_attr_t rssi)
{
	uint8_t data_length = 0;
	uint8_t header;

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(source, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming frame from %s (header 0x%02x)", addr_str, payload[0]);

	/* Notify the discovery module, that we have seen a peer */
	if (!source->isIP) {
		DISCOVERY.alive((linkaddr_t*)&source->lowpan);
	}
	// TODO send alive for ip too
	// change type of addr to cl_addr_t
	// onyl saving this value in discovery list

	/* Check the COMPAT information */
	if( (payload[0] & CONVERGENCE_LAYER_MASK_COMPAT) != CONVERGENCE_LAYER_COMPAT ) {
		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_INF, "Ignoring incoming frame from %s", addr_str);
		return -1;
	}

	header = payload[0];
	const uint8_t* const data_pointer = payload + 1;
	data_length = length - 1;

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_DATA ) {
		/* is data */
		const int flags = (header & CONVERGENCE_LAYER_MASK_FLAGS) >> 0;
		const int sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		return convergence_layer_incoming_data(source, data_pointer, data_length, rssi, sequence_number, flags);
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_DISCOVERY ) {
		/* is discovery */
		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming discovery frame from %s", addr_str);

		// TODO change receive for cl_addr_t
		if (!source->isIP) {
			// TODO make linkaddr const and rmeove cast
			DISCOVERY.receive((linkaddr_t*)&source->lowpan, (uint8_t*)data_pointer, data_length);
		}

		return 1;
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_ACK ) {
		/* is ACK */
		int flags = 0;
		int sequence_number = 0;

		flags = (header & CONVERGENCE_LAYER_MASK_FLAGS) >> 0;
		sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming Ack frame from %s with SeqNo %u", addr_str, sequence_number);

		convergence_layer_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_ACK, flags);

		return 1;
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_NACK ) {
		/* is NACK */
		int flags = 0;
		int sequence_number = 0;

		flags = (header & CONVERGENCE_LAYER_MASK_FLAGS) >> 0;
		sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming Nack frame from %s with SeqNo %u", addr_str, sequence_number);

		convergence_layer_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_NACK, flags);

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
		/* If we did not send (channel busy) and it was a bundle, then slow
		 * the transmission rate down a bit by using a timer. Otherwise:
		 * poll the process.
		 */
		if( outcome == CONVERGENCE_LAYER_STATUS_NOSEND && pointer != NULL ) {
			/* Use timer to slow the stuff down */
			convergence_layer_backoff_pending = true;
			xTaskNotify(convergence_layer_task, 0, eNoAction);
		} else {
			/* Poll to make it faster */
			xTaskNotify(convergence_layer_task, 0, eNoAction);
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

	/* Unset IN_TRANSIT flag */
	ticket->flags &= ~CONVERGENCE_LAYER_QUEUE_IN_TRANSIT;

	/* Bundle was transmitted successfully */
	if( outcome == CONVERGENCE_LAYER_STATUS_OK ) {
		// Bundle is sent, now waiting for the app-layer ACK
		ticket->flags |= CONVERGENCE_LAYER_QUEUE_ACTIVE | CONVERGENCE_LAYER_QUEUE_ACK_PEND;

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
		ticket->flags |= CONVERGENCE_LAYER_QUEUE_FAIL;

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
	ticket->flags |= CONVERGENCE_LAYER_QUEUE_ACTIVE;

	return 1;
}

int convergence_layer_delete_bundle(uint32_t bundle_number)
{
	struct transmit_ticket_t * ticket = NULL;
	int changed = 1;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Deleting tickets for bundle %lu", bundle_number);

	while(changed) {
		changed = 0;

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

		changed = 1;
	}


	return 1;
}

int convergence_layer_is_blocked(const cl_addr_t* const neighbour)
{
	struct blocked_neighbour_t * n = NULL;

	for( n = list_head(blocked_neighbour_list);
		 n != NULL;
		 n = list_item_next(n) ) {
		if( cl_addr_cmp(neighbour, &n->neighbour) ) {
			return 1;
		}
	}

	return 0;
}

int convergence_layer_set_blocked(const cl_addr_t* const neighbour)
{
	struct blocked_neighbour_t * n = NULL;

	n = memb_alloc(&blocked_neighbour_mem);
	if( n == NULL ) {
		LOG(LOGD_DTN, LOG_CL, LOGL_ERR, "Cannot allocate neighbour memory");
		return -1;
	}

	/* Fill the struct */
	cl_addr_copy(&n->neighbour, neighbour);
	n->timestamp = xTaskGetTickCount();

	/* Add it to the list */
	list_add(blocked_neighbour_list, n);

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(neighbour, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Block neighbour %s", addr_str);
	return 1;
}

int convergence_layer_set_unblocked(const cl_addr_t* const neighbour)
{
	struct blocked_neighbour_t * n = NULL;

	for( n = list_head(blocked_neighbour_list);
		 n != NULL;
		 n = list_item_next(n) ) {
		if( cl_addr_cmp(neighbour, &n->neighbour) ) {
			list_remove(blocked_neighbour_list, n);
			memb_free(&blocked_neighbour_mem, n);

			char addr_str[CL_ADDR_STRING_LENGTH];
			cl_addr_string(neighbour, addr_str, sizeof(addr_str));
			LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Unblock neighbour %s", addr_str);
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

		if( (xTaskGetTickCount() - n->timestamp) >= pdMS_TO_TICKS(CONVERGENCE_LAYER_TIMEOUT * 1000) ) {
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
		if( cl_addr_cmp(&ticket->neighbour, &n->neighbour) && (ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK_PEND) ) {
			break;
		}
	}

	/* Unblock the neighbour */
	convergence_layer_set_unblocked(&n->neighbour);

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(&n->neighbour, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "Neighbour %s stale, removing lock", addr_str);

	/* There seems to be no ticket, nothing to do for us */
	if( ticket == NULL ) {
		return;
	}

	/* Otherwise: just reactivate the ticket, it will be transmitted again */
	ticket->flags |= CONVERGENCE_LAYER_QUEUE_ACTIVE;

	if( convergence_layer_pending == 0 ) {
		/* Tell the process to resend the bundles */
		xTaskNotify(convergence_layer_task, 0, eNoAction);
	}
}

void check_blocked_tickets() {
	struct transmit_ticket_t * ticket = NULL;
	int changed = 1;

	while(changed) {
		changed = 0;

		for( ticket = list_head(transmission_ticket_list);
			 ticket != NULL;
			 ticket = list_item_next(ticket) ) {

			/* Only multipart receiver tickets can time out */
			if( !(ticket->flags & CONVERGENCE_LAYER_QUEUE_MULTIPART_RECV) ) {
				continue;
			}

			if( (xTaskGetTickCount() - ticket->timestamp) > pdMS_TO_TICKS(CONVERGENCE_LAYER_MULTIPART_TIMEOUT * 1000) ) {
				char addr_str[CL_ADDR_STRING_LENGTH];
				cl_addr_string(&ticket->neighbour, addr_str, sizeof(addr_str));
				LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Multipart receiving ticket for peer %s timed out, removing", addr_str);

				changed = 1;
				convergence_layer_free_transmit_ticket(ticket);
				break;
			}
		}
	}
}

int convergence_layer_neighbour_down(const cl_addr_t* const neighbour)
{
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

			/* Do not delete tickets for which we are currently awaiting an ACK */
			if( ticket->flags & CONVERGENCE_LAYER_QUEUE_ACK_PEND ) {
				continue;
			}

			/* Multipart receiving tickets must not be signalled to routing */
			if( ticket->flags & CONVERGENCE_LAYER_QUEUE_MULTIPART_RECV ) {
				/* Mark as changed */
				changed = 1;

				/* Free ticket */
				convergence_layer_free_transmit_ticket(ticket);

				/* Stop look and start over again */
				break;
			}

			if( cl_addr_cmp(neighbour, &ticket->neighbour) ) {
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

		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(&ticket->neighbour, addr_str, sizeof(addr_str));
		printf("B %10lu to %s with SeqNo %u and Flags %02X\n", ticket->bundle_number, addr_str, ticket->sequence_number, ticket->flags);
	}

	printf("---\n");
}

static void convergence_layer_process(void* p)
{
	struct transmit_ticket_t * ticket = NULL;
	int n;

	/* Initialize ticket storage */
	memb_init(&transmission_ticket_mem);
	list_init(transmission_ticket_list);

	/* Initialize neighbour storage */
	memb_init(&blocked_neighbour_mem);
	list_init(blocked_neighbour_list);

	LOG(LOGD_DTN, LOG_CL, LOGL_INF, "CL process is running");

	while(1) {
		const BaseType_t notification_received = xTaskNotifyWait( 0, 0, NULL, pdMS_TO_TICKS(1000) );
		// TODO für convergence_layer_backoff besondere notification verwenden und dann mit vTaskDelay 100ms warten
		// wenn andere notification dann möglicherweise direkt ausführen

		// TODO timer nicht immer neu aufrufen sondern methoden immer anch genau 1000ms ausführen
		// vielleicht eignenen prozess verwenden
// TODO		if(!notification_received) {
			check_blocked_neighbours();
			check_blocked_tickets();
// TODO		} else
			{
			/* slow down the transmission to mind collisions */
			if (convergence_layer_backoff_pending) {
				vTaskDelay( pdMS_TO_TICKS(100) );
				convergence_layer_backoff_pending = false;
			}

			convergence_layer_pending = 0;


			/* If we are currently transmitting, we cannot send another bundle */
			if( convergence_layer_transmitting ) {
				/* We will get polled again when the MAC layers calls us back,
				 * so lean back and relax
				 */
				continue;
			}


			LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Try to send %d available tickets", list_length(transmission_ticket_list));

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

				/* Tickets that are in transit have to wait */
				if( ticket->flags & CONVERGENCE_LAYER_QUEUE_IN_TRANSIT ) {
					continue;
				}

				/* Tickets that are in any other state than ACTIVE cannot be transmitted */
				if( !(ticket->flags & CONVERGENCE_LAYER_QUEUE_ACTIVE) ) {
					continue;
				}

				/* Neighbour for which we are currently waiting on app-layer ACKs cannot receive anything now */
				if( convergence_layer_is_blocked(&ticket->neighbour) ) {
					char addr_str[CL_ADDR_STRING_LENGTH];
					cl_addr_string(&ticket->neighbour, addr_str, sizeof(addr_str));
					LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Neighbour %s is blocked. Not sending bundle %u with ticket %p",
						addr_str, ticket->bundle_number, ticket);
					continue;
				}

				/* Send the bundle just now */
				convergence_layer_dgram_prepare_segmentation(ticket);

				/* Radio is busy now, defer */
				break;
			}
		}
	}
}
