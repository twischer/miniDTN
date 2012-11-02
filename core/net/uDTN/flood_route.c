/**
 * \addtogroup routing
 * @{
 */

 /**
 * \defgroup floodrouting Flooding Routing module
 *
 * @{
 */

/**
 * \file 
 * implementation of flooding
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
 */
#include <string.h>

#include "net/netstack.h"
#include "net/rime/rimeaddr.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "contiki.h"
#include "clock.h"

#include "bundle.h"
#include "storage.h"
#include "sdnv.h"
#include "agent.h"
#include "discovery.h"
#include "statistics.h"
#include "bundleslot.h"
#include "delivery.h"
#include "logging.h"
#include "convergence_layer.h"

#include "routing.h"

#define BLACKLIST_TIMEOUT	10
#define BLACKLIST_THRESHOLD	3
#define BLACKLIST_SIZE		3

struct blacklist_entry_t {
	struct blacklist_entry_t * next;

	rimeaddr_t node;
	uint8_t counter;
	uint16_t timestamp;
};

struct routing_list_entry_t {
	/** pointer to the next entry */
	struct routing_list_entry_t * next;

	/** pointer to MMEM containing the routing_entry_t */
	struct mmem entry;
};

struct routing_entry_t {
	/** number of the bundle */
	uint32_t bundle_number;

	/** bundle flags */
	uint8_t flags;

	/** number of nodes the bundle has been sent to already */
	uint8_t send_to;

	/** addresses of nodes this bundle was sent to */
	rimeaddr_t neighbours[ROUTING_NEI_MEM];

	/** bundle destination */
	uint32_t destination_node;

	/** bundle source */
	uint32_t source_node;

	/** neighbour from which we have received the bundle */
	rimeaddr_t received_from_node;
};

/**
 * Routing process
 */
PROCESS(routing_process, "FLOOD ROUTE process");

MEMB(blacklist_mem, struct blacklist_entry_t, BLACKLIST_SIZE);
LIST(blacklist_list);

MEMB(routing_mem, struct routing_list_entry_t, BUNDLE_STORAGE_SIZE);
LIST(routing_list);

uint8_t flood_send_to_known(void);

/**
 * \brief Adds (or refreshes) the entry of 'neighbour' on the blacklist
 */
int flood_blacklist_add(rimeaddr_t * neighbour)
{
	struct blacklist_entry_t * entry;

	for(entry = list_head(blacklist_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		if( rimeaddr_cmp(neighbour, &entry->node) ) {
			if( (clock_time() - entry->timestamp) > (BLACKLIST_TIMEOUT * CLOCK_SECOND) ) {
				// Reusing existing (timedout) entry
				entry->counter = 0;
			}

			entry->counter ++;
			entry->timestamp = clock_time();

			if( entry->counter > BLACKLIST_THRESHOLD ) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "%u.%u blacklisted", neighbour->u8[0], neighbour->u8[1]);
				return 1;
			}

			// Found but not blacklisted
			return 0;
		}
	}

	entry = memb_alloc(&blacklist_mem);

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_WRN, "Cannot allocate memory for blacklist");
		return 0;
	}

	rimeaddr_copy(&entry->node, neighbour);
	entry->counter = 1;
	entry->timestamp = clock_time();

	list_add(blacklist_list, entry);

	return 0;
}

/**
 * \brief Deletes a neighbour from the blacklist
 */
void flood_blacklist_delete(rimeaddr_t * neighbour)
{
	struct blacklist_entry_t * entry;

	for(entry = list_head(blacklist_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		if( rimeaddr_cmp(neighbour, &entry->node) ) {
			list_remove(blacklist_list, entry);
			memset(entry, 0, sizeof(struct blacklist_entry_t));
			memb_free(&blacklist_mem, entry);
			return;
		}
	}
}

/**
* \brief called by agent at startup
*/
void flood_init(void)
{
	// Start CL process
	process_start(&routing_process, NULL);
}

/**
* \brief checks if there are bundle to send to dest
* \param dest pointer to the address of the new neighbor
*/
void flood_new_neigh(rimeaddr_t *dest)
{
	flood_schedule_resubmission();
}


void flood_schedule_resubmission(void)
{
	process_poll(&routing_process);
}

uint8_t flood_send_to_local(void)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;
	struct mmem * bundlemem = NULL;
	int delivered = 0;

	for( n = (struct routing_list_entry_t *) list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {
		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry == NULL ) {
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "broken MMEM structure for bundle %lu", entry->bundle_number);
			continue;
		}

		// Should this bundle be delivered locally?
		if( (entry->flags & ROUTING_FLAG_LOCAL) && !(entry->flags & ROUTING_FLAG_IN_DELIVERY) ) {
			bundlemem = BUNDLE_STORAGE.read_bundle(entry->bundle_number);
			if( bundlemem == NULL ) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "cannot read bundle %lu", entry->bundle_number);
				continue;
			}

			if( deliver_bundle(bundlemem) ) {
				entry->flags |= ROUTING_FLAG_IN_DELIVERY;
				delivered ++;
			}
		}
	}

	return delivered;
}

int flood_send_bundle(uint32_t bundle_number, rimeaddr_t neighbour)
{
	struct transmit_ticket_t * ticket = NULL;

	/* Allocate a transmission ticket */
	ticket = convergence_layer_get_transmit_ticket();
	if( ticket == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_WRN, "unable to allocate transmit ticket");
		return -1;
	}

	/* Specify which bundle */
	rimeaddr_copy(&ticket->neighbour, &neighbour);
	ticket->bundle_number = bundle_number;

	/* Put the bundle in the queue */
	convergence_layer_enqueue_bundle(ticket);

	return 1;
}

uint8_t flood_send_to_known(void)
{
	struct discovery_neighbour_list_entry *nei_l = NULL;
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;
	int h = 0;

	/* First: deliver bundles to local services */
	flood_send_to_local();

	LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "send to known neighbours");

	/**
	 * It is likely, that we will have less neighbours than bundles - therefore, we want to to go through bundles only once
	 */
	for( n = (struct routing_list_entry_t *) list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);
		if( entry == NULL ) {
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_WRN, "Bundle with invalid MMEM structure");
		}

		/* Skip this bundle, if it is not queued for forwarding */
		if( !(entry->flags & ROUTING_FLAG_FORWARD) || (entry->flags & ROUTING_FLAG_IN_TRANSIT) ) {
			continue;
		}

		/* Who is the destination for this bundle? */
		rimeaddr_t dest_node = convert_eid_to_rime(entry->destination_node);

		/* First step: check, if the destination is one of our neighbours */
		for( nei_l = DISCOVERY.neighbours();
			 nei_l != NULL;
			 nei_l = list_item_next(nei_l) ) {

			if( rimeaddr_cmp(&nei_l->neighbour, &dest_node) ) {
				break;
			}
		}

		if( nei_l != NULL ) {
			/* We know the neighbour, send it directly */
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "send bundle %lu to %u:%u directly", entry->bundle_number, nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

			/* Mark bundle as busy */
			entry->flags |= ROUTING_FLAG_IN_TRANSIT;

			/* And queue it for sending */
			h = flood_send_bundle(entry->bundle_number, nei_l->neighbour);
			if( h < 0 ) {
				/* Enqueuing bundle failed - unblock it */
				entry->flags &= ~ROUTING_FLAG_IN_TRANSIT;

				/* If sending the bundle fails, all other will likely also fail */
				return 1;
			}

			/* We do not want the bundle to be sent to anybody else at the moment, so: */
			continue;
		}

		/* At this point, we know that the bundle is not for one of our neighbours, so send it to all the others */
		for( nei_l = DISCOVERY.neighbours();
			 nei_l != NULL;
			 nei_l = list_item_next(nei_l) ) {
			int sent = 0;
			int i;

			rimeaddr_t source_node = convert_eid_to_rime(entry->source_node);
			if( rimeaddr_cmp(&nei_l->neighbour, &source_node) ) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "not sending bundle to originator");

				/* Go on with the next neighbour */
				continue;
			}

			if( rimeaddr_cmp(&nei_l->neighbour, &entry->received_from_node) ) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "not sending back to sender");

				/* Go on with the next neighbour */
				continue;
			}

			/* Did we forward the bundle to that neighbour already? */
			for (i = 0 ; i < ROUTING_NEI_MEM ; i++) {
				if ( rimeaddr_cmp(&entry->neighbours[i], &nei_l->neighbour)){
					LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %lu already sent to node %u:%u!", entry->bundle_number, entry->neighbours[i].u8[0], entry->neighbours[i].u8[1]);
					sent = 1;

					// Break the (narrowest) for
					break;
				}
			}

			if(!sent) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "send bundle %lu to %u:%u", entry->bundle_number, nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

				/* Mark bundle as busy */
				entry->flags |= ROUTING_FLAG_IN_TRANSIT;

				/* And queue it for sending */
				h = flood_send_bundle(entry->bundle_number, nei_l->neighbour);
				if( h < 0 ) {
					/* Enqueuing bundle failed - unblock it */
					entry->flags &= ~ROUTING_FLAG_IN_TRANSIT;

					/* If sending the bundle fails, all other will likely also fail */
					return 1;
				}
			}
		}
	}

	return 0;
}

/**
 * Wrapper function for agent calls to resubmit bundles for already known neighbours
 */
void flood_resubmit_bundles() {
	flood_schedule_resubmission();
}

/**
 * \brief Checks whether a bundle still has to be kept or can be deleted
 * \param bundle_number Number of the bundle
 */
void flood_check_keep_bundle(uint32_t bundle_number) {
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;

	// Now we have to find the appropriate Storage struct
	for( n = (struct routing_list_entry_t *) list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry->bundle_number == bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "Bundle not in storage yet");
		return;
	}

	if( (entry->flags & ROUTING_FLAG_LOCAL) || (entry->flags & ROUTING_FLAG_FORWARD) ) {
		return;
	}

	LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "Deleting bundle %u", entry->bundle_number);
	BUNDLE_STORAGE.del_bundle(entry->bundle_number, REASON_DELIVERED);
}

/**
* \brief Adds a new bundle to the list of bundles
* \param bundle_number bundle number of the bundle
* \return >0 on success, <0 on error
*/
int flood_new_bundle(uint32_t bundle_number)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;
	struct mmem * bundlemem = NULL;
	struct bundle_t * bundle = NULL;

	// Let us see, if we know this bundle already
	for( n = list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry->bundle_number == bundle_number ) {
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "agent announces bundle %lu that is already known", bundle_number);
			return -1;
		}
	}

	// Notify statistics
	statistics_bundle_incoming(1);

	// Now allocate new memory for the list entry
	n = memb_alloc(&routing_mem);
	if( n == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "cannot allocate list entry for bundle, please increase BUNDLE_STORAGE_SIZE");
		return -1;
	}

	memset(n, 0, sizeof(struct routing_list_entry_t));

	// Now allocate new MMEM memory for the struct in the list
	if( !mmem_alloc(&n->entry, sizeof(struct routing_entry_t)) ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "cannot allocate routing struct for bundle, MMEM is full");
		memb_free(&routing_mem, n);
		return -1;
	}

	// Now go and request the bundle from storage
	bundlemem = BUNDLE_STORAGE.read_bundle(bundle_number);
	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "unable to read bundle %lu", bundle_number);
		mmem_free(&n->entry);
		memb_free(&routing_mem, n);
		return -1;
	}

	// Get our bundle struct and check the pointer
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "invalid bundle pointer for bundle %lu", bundle_number);
		mmem_free(&n->entry);
		memb_free(&routing_mem, n);
		bundle_dec(bundlemem);
		return -1;
	}

	// Now we have our entry
	// We have to get the pointer AFTER getting the bundle from storage, because accessing the
	// storage may change the MMEM structure and thus the pointers!
	entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);
	memset(entry, 0, sizeof(struct routing_entry_t));

	// Nothing can go wrong anymore, add the (surrounding) struct to the list
	list_add(routing_list, n);

	// If we have a bundle for our node, mark the bundle
	if( bundle->dst_node == (uint32_t) dtn_node_id ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle is for local");
		entry->flags |= ROUTING_FLAG_LOCAL;

		if( bundle->flags & BUNDLE_FLAG_SINGLETON ) {
			// Apparently the bundle is *only* for us
			entry->flags &= ~ROUTING_FLAG_FORWARD;
		} else {
			// Bundle is also for somebody else
			entry->flags |= ROUTING_FLAG_FORWARD;
		}
	} else {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle is for forward");
		// Bundle is not for us, only for forwarding
		entry->flags |= ROUTING_FLAG_FORWARD;
	}

	// Now copy the necessary attributes from the bundle
	entry->bundle_number = bundle_number;
	get_attr(bundlemem, DEST_NODE, &entry->destination_node);
	get_attr(bundlemem, SRC_NODE, &entry->source_node);
	rimeaddr_copy(&entry->received_from_node, &bundle->msrc);

	// Now that we have the bundle, we do not need the allocated memory anymore
	bundle_dec(bundlemem);

	// Schedule to deliver and forward the bundle
	flood_schedule_resubmission();

	// We do not have a failure here, so it must be a success
	return 1;
}

/**
* \brief deletes bundle from list
* \param bundle_num bundle nuber of the bundle
*/
void flood_del_bundle(uint32_t bundle_number)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;

	// Find the bundle in our internal storage
	for( n = list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry->bundle_number == bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "flood_del_bundle for bundle %lu that we do not know", bundle_number);
		return;
	}

	memset(MMEM_PTR(&n->entry), 0, sizeof(struct routing_entry_t));

	// Free up the memory for the struct
	mmem_free(&n->entry);

	list_remove(routing_list, n);

	memset(n, 0, sizeof(struct routing_list_entry_t));

	// And also free the memory for the list entry
	memb_free(&routing_mem, n);
}

/**
* \brief callback function sets the status of a bundle in the list
* \param route pointer to route struct 
* \param status status code
* \num_tx number of retransmissions 
*/
void flood_sent(struct transmit_ticket_t * ticket, uint8_t status)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;

	// Find the bundle in our internal storage
	for( n = list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry->bundle_number == ticket->bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "Bundle not in storage yet");
		return;
	}

	// Tell the agent to call us again to resubmit bundles
	flood_schedule_resubmission();

	/* Bundle is not busy anymore */
	entry->flags &= ~ROUTING_FLAG_IN_TRANSIT;

	if( status == ROUTING_STATUS_NACK ||
		status == ROUTING_STATUS_FAIL ) {
		// NACK = Other side rejected the bundle, try again later
		// FAIL = Transmission failed
		// --> note down address in blacklist
		if( flood_blacklist_add(&ticket->neighbour) ) {
			// Node is now past threshold and blacklisted, notify discovery
			DISCOVERY.dead(&ticket->neighbour);
			flood_blacklist_delete(&ticket->neighbour);
		}

		/* Free up the ticket */
		convergence_layer_free_transmit_ticket(ticket);

		return;
	}

	// Here: status == ROUTING_STATUS_OK
	statistics_bundle_outgoing(1);

	flood_blacklist_delete(&ticket->neighbour);

	rimeaddr_t dest_n = convert_eid_to_rime(entry->destination_node);
	if (rimeaddr_cmp(&ticket->neighbour, &dest_n)) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle sent to destination node");
		uint32_t bundle_number = ticket->bundle_number;

		/* Free up the ticket */
		convergence_layer_free_transmit_ticket(ticket);
		ticket = NULL;

		// Unset the forward flag
		entry->flags &= ~ROUTING_FLAG_FORWARD;
		flood_check_keep_bundle(bundle_number);

		return;
	} else {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle for %u.%u delivered to %u.%u", dest_n.u8[0], dest_n.u8[1], ticket->neighbour.u8[0], ticket->neighbour.u8[1]);
	}

	if (entry->send_to < ROUTING_NEI_MEM) {
		rimeaddr_copy(&entry->neighbours[entry->send_to], &ticket->neighbour);
		entry->send_to++;
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %lu sent to %u nodes", ticket->bundle_number, entry->send_to);
	} else if (entry->send_to >= ROUTING_NEI_MEM) {
		// Here we can delete the bundle from storage, because it will not be routed anyway
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %lu sent to max number of nodes, deleting", ticket->bundle_number);

		// Unset the forward flag
		entry->flags &= ~ROUTING_FLAG_FORWARD;
		flood_check_keep_bundle(ticket->bundle_number);
	}

	/* Free up the ticket */
	convergence_layer_free_transmit_ticket(ticket);
}

/**
 * \brief Incoming notification, that service has finished processing bundle
 * \param bundle_num Number of the bundle
 */
void flood_locally_delivered(struct mmem * bundlemem) {
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;
	struct bundle_t * bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "flood_locally_delivered called with invalid pointer");
		return;
	}

	// Find the bundle in our internal storage
	for( n = (struct routing_list_entry_t *) list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry->bundle_number == bundle->bundle_num ) {
			break;
		}
	}

	if( n == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "Bundle not in storage yet");
		return;
	}

	// Unset the IN_DELIVERY flag
	entry->flags &= ~ROUTING_FLAG_IN_DELIVERY;

	// Unset the LOCAL flag
	entry->flags &= ~ROUTING_FLAG_LOCAL;

	// Unblock the receiving service
	unblock_service(bundlemem);

	// Free the bundle memory
	bundle_dec(bundlemem);

	// Check remaining live of bundle
	flood_check_keep_bundle(entry->bundle_number);

	// Tell the agent to call us again to resubmit bundles
	flood_schedule_resubmission();
}

PROCESS_THREAD(routing_process, ev, data)
{
	PROCESS_BEGIN();

	LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "FLOOD ROUTE process in running");

	// Initialize memory used to store blacklisted neighbours
	memb_init(&blacklist_mem);
	list_init(blacklist_list);

	// Initialize memory used to store bundles for routing
	memb_init(&routing_mem);
	list_init(routing_list);

	while(1) {
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

		flood_send_to_known();
	}

	PROCESS_END();
}

const struct routing_driver flood_route ={
	"flood_route",
	flood_init,
	flood_new_neigh,
	flood_new_bundle,
	flood_del_bundle,
	flood_sent,
	flood_resubmit_bundles,
	flood_locally_delivered,
};

