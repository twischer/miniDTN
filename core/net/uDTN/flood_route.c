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
#include "net/netstack.h"


#include "dtn_config.h"
#include "bundle.h"
#include "net/rime/rimeaddr.h"
#include "storage.h"
#include "sdnv.h"
#include "routing.h"
#include "agent.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "contiki.h"
#include <string.h>
#include "discovery.h"
#include "bundle.h"
#include "storage.h"
#include "statistics.h"
#include "clock.h"
#include "bundleslot.h"
#include "delivery.h"
#include "logging.h"
#include "dtn_config.h"

#define DEBUG 0
#if DEBUG 
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define BLACKLIST_TIMEOUT	10
#define BLACKLIST_THRESHOLD	3
#define BLACKLIST_SIZE		3

uint8_t flood_transmitting;
uint8_t flood_agent_event_pending = 0;
struct route_t route;

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
	uint16_t bundle_number;

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

MEMB(blacklist_mem, struct blacklist_entry_t, BLACKLIST_SIZE);
LIST(blacklist_list);

MEMB(routing_mem, struct routing_list_entry_t, BUNDLE_STORAGE_SIZE);
LIST(routing_list);

uint8_t flood_sent_to_known(void);

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
	LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "init flood_route");

	// Initialize memory used to store blacklisted neighbours
	memb_init(&blacklist_mem);
	list_init(blacklist_list);

	// Initialize memory used to store bundles for routing
	memb_init(&routing_mem);
	list_init(routing_list);

	flood_transmitting = 0;
	flood_agent_event_pending = 0;
}

/**
* \brief checks if there are bundle to send to dest
* \param dest pointer to the address of the new neighbor
*/
void flood_new_neigh(rimeaddr_t *dest)
{
	flood_sent_to_known();
}


void flood_delete_list(void)
{
}

void flood_schedule_resubmission(void)
{
	if( !flood_agent_event_pending ) {
		flood_agent_event_pending = 1;
		// Tell the agent to call us again to resubmit bundles
		process_post(&agent_process, dtn_bundle_resubmission_event, NULL);
	}
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

		// Should this bundle be delivered locally?
		if( (entry->flags & ROUTING_FLAG_LOCAL) && !(entry->flags & ROUTING_FLAG_IN_DELIVERY) ) {
			bundlemem = BUNDLE_STORAGE.read_bundle(entry->bundle_number);
			if( bundlemem == NULL ) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "cannot read bundle %d", entry->bundle_number);
				bundlemem = NULL;
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

uint8_t flood_sent_to_known(void)
{
	struct discovery_neighbour_list_entry *nei_l = NULL;
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;

	// First: deliver bundles to local services
	flood_send_to_local();

	if( flood_transmitting ) {
		flood_schedule_resubmission();
		return 0;
	}

	LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "send to known neighbours");

	/**
	 * First step: look, if we know the direct destination already
	 * If so, always use direct delivery, never send to another node
	 */
	for(nei_l = DISCOVERY.neighbours(); nei_l != NULL; nei_l = list_item_next(nei_l)) {
		// Now go through all bundles
		for( n = (struct routing_list_entry_t *) list_head(routing_list);
			 n != NULL;
			 n = list_item_next(n) ) {
			entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

			// Skip this bundle, if it is not queued for forwarding
			if( !(entry->flags & ROUTING_FLAG_FORWARD) ) {
				continue;
			}

			// Who is the destination for this bundle?
			rimeaddr_t dest_node = convert_eid_to_rime(entry->destination_node);

			if( rimeaddr_cmp(&nei_l->neighbour, &dest_node) ) {
				// We know the neighbour, send it directly
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "send bundle %u to %u:%u directly", entry->bundle_number, nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

				// Clear memory to store the route
				memset(&route, 0, sizeof(struct route_t));

				// Save next hop and bundle number
				rimeaddr_copy(&route.dest, &nei_l->neighbour);
				route.bundle_num = entry->bundle_number;

				// Lock ourself
				flood_transmitting = 1;

				// And tell the agent to transmit
				agent_send_bundles(&route);

				// Only one bundle at a time
				return 1;
			}
		}
	}

	/**
	 * If we do not happen to have the destination as neighbour,
	 * flood it to everyone
	 */
	for(nei_l = DISCOVERY.neighbours(); nei_l != NULL; nei_l = list_item_next(nei_l)) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "neighbour %u.%u", nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

		for( n = (struct routing_list_entry_t *) list_head(routing_list);
			 n != NULL;
			 n = list_item_next(n) ) {
			entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

			// Skip this bundle, if it is not queued for forwarding
			if( !(entry->flags & ROUTING_FLAG_FORWARD) ) {
				continue;
			}

			LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "Bundle %u, SRC %lu, DEST %lu, MSRC %u.%u", entry->bundle_number,  entry->source_node, entry->destination_node, entry->received_from_node.u8[0], entry->received_from_node.u8[1]);

			uint8_t i, sent = 0;

			rimeaddr_t source_node = convert_eid_to_rime(entry->source_node);
			if( rimeaddr_cmp(&nei_l->neighbour, &source_node) ) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "not sending bundle to originator");

				// Go on with the next bundle
				continue;
			}

			if( rimeaddr_cmp(&nei_l->neighbour, &entry->received_from_node) ) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "not sending back to sender");

				// Go on with the next bundle
				continue;
			}

			for (i = 0 ; i < ROUTING_NEI_MEM ; i++) {
				if ( rimeaddr_cmp(&entry->neighbours[i], &nei_l->neighbour)){
					LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %u already sent to node %u:%u!", entry->bundle_number, entry->neighbours[i].u8[0], entry->neighbours[i].u8[1]);
					sent = 1;

					// Break the (narrowest) for
					break;
				}
			}

			if(!sent) {
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "send bundle %u to %u:%u", entry->bundle_number, nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

				// Clear memory to store the route
				memset(&route, 0, sizeof(struct route_t));

				// Save next hop and bundle number
				rimeaddr_copy(&route.dest, &nei_l->neighbour);
				route.bundle_num = entry->bundle_number;

				// Lock ourself
				flood_transmitting = 1;

				// And tell the agent to transmit
				agent_send_bundles(&route);

				// Only one bundle at a time
				return 1;
			}
		}
	}

	return 0;
}

/**
 * Wrapper function for agent calls to resubmit bundles for already known neighbours
 */
void flood_resubmit_bundles(uint8_t called_by_event) {
	if( called_by_event == 1 ) {
		flood_agent_event_pending = 0;
	}

	flood_sent_to_known();
}

/**
 * \brief Checks whether a bundle still has to be kept or can be deleted
 * \param bundle_number Number of the bundle
 */
void flood_check_keep_bundle(uint16_t bundle_number) {
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
int flood_new_bundle(uint16_t bundle_number)
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
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "agent announces bundle %d that is already known", bundle_number);
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

	// Now allocate new MMEM memory for the struct in the list
	if( !mmem_alloc(&(n->entry), sizeof(struct routing_entry_t)) ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "cannot allocate routing struct for bundle, MMEM is full");
		memb_free(&routing_mem, n);
		return -1;
	}

	// Now go and request the bundle from storage
	bundlemem = BUNDLE_STORAGE.read_bundle(bundle_number);
	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "unable to read bundle %d", bundle_number);
		mmem_free(&n->entry);
		memb_free(&routing_mem, n);
		return -1;
	}

	// Get our bundle struct and check the pointer
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "invalid bundle pointer for bundle %d", bundle_number);
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
void flood_del_bundle(uint16_t bundle_number)
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
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "flood_del_bundle for bundle %d that we do not know", bundle_number);
		return;
	}

	// Free up the memory for the struct
	mmem_free(&n->entry);

	// And also free the memory for the list entry
	memb_free(&routing_mem, n);
}

/**
* \brief callback function sets the status of a bundle in the list
* \param route pointer to route struct 
* \param status status code
* \num_tx number of retransmissions 
*/
void flood_sent(struct route_t *route, int status, int num_tx)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;

	// Release transmit lock
	flood_transmitting = 0;

	// Find the bundle in our internal storage
	for( n = list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {
		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry->bundle_number == route->bundle_num ) {
			break;
		}
	}

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "Bundle not in storage yet");
		return;
	}

	switch(status) {
	case MAC_TX_ERR:
		// This is the default state value of nullrdc, if the return value of NETSTACK_RADIO.transmit is none of RADIO_TX_OK and RADIO_TX_COLLISION applies.
		// Unfortunately, we cannot distinguish the original errors here
	case MAC_TX_COLLISION:
		// This status can either mean, that the packet could not be transmitted because another packet is pending
		// Or it means that the packet was transmitted, but an ack for another packet was received
		// It seems to even mean, that CSMA will try a retransmit, but we cannot distinguish that
		// In any case, retransmitting the packet seems to be the more sensible choice
	case MAC_TX_NOACK:
		// This status should never come back for CSMA, NULLRDC and RF230BB

		// dtn-network tried to send bundle but failed, has to be retried
		if( status == MAC_TX_ERR ) {
			PRINTF("FLOOD: MAC_TX_ERR\n");
		} else if( status == MAC_TX_COLLISION ) {
			PRINTF("FLOOD: collision after %d tx\n", num_tx);
		} else if( status == MAC_TX_NOACK ) {
			PRINTF("FLOOD: noack after %d tx\n", num_tx);
		}

		// Transmission failed, note down address in blacklist
		if( flood_blacklist_add(&route->dest) ) {
			// Node is now past threshold and blacklisted, notify discovery
			DISCOVERY.dead(&route->dest);
			flood_blacklist_delete(&route->dest);
		}

		break;

	case MAC_TX_OK:
		PRINTF("FLOOD: sent after %d tx\n", num_tx);
		statistics_bundle_outgoing(1);

		flood_blacklist_delete(&route->dest);

		rimeaddr_t dest_n = convert_eid_to_rime(entry->destination_node);
		if (rimeaddr_cmp(&route->dest, &dest_n)) {
			PRINTF("FLOOD: bundle sent to destination node\n");

			// Unset the forward flag
			entry->flags &= ~ROUTING_FLAG_FORWARD;
			flood_check_keep_bundle(route->bundle_num);

			break;
		} else {
			PRINTF("FLOOD: bundle for %u:%u delivered to %u:%u\n", dest_n.u8[0], dest_n.u8[1], route->dest.u8[0], route->dest.u8[1]);
		}

		if (entry->send_to < ROUTING_NEI_MEM) {
			rimeaddr_copy(&entry->neighbours[entry->send_to], &route->dest);
			entry->send_to++;
			PRINTF("FLOOD: bundle %u sent to %u nodes\n", route->bundle_num, entry->send_to);
	    } else if (entry->send_to >= ROUTING_NEI_MEM) {
	    	// Here we can delete the bundle from storage, because it will not be routed anyway
			PRINTF("FLOOD: bundle sent to max number of nodes\n");

			// Unset the forward flag
			entry->flags &= ~ROUTING_FLAG_FORWARD;
			flood_check_keep_bundle(route->bundle_num);
		}

	    break;

	default:
	    printf("FLOOD: error %d after %d tx\n", status, num_tx);
	    break;
	}

	// Tell the agent to call us again to resubmit bundles
	flood_schedule_resubmission();
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
}

const struct routing_driver flood_route ={
	"flood_route",
	flood_init,
	flood_new_neigh,
	flood_new_bundle,
	flood_del_bundle,
	flood_sent,
	flood_delete_list,
	flood_resubmit_bundles,
	flood_locally_delivered,
};

