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
#include "r_storage.h"
#include "statistics.h"

#define DEBUG 0
#if DEBUG 
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint8_t flood_transmitting;
uint8_t flood_agent_event_pending = 0;

MEMB(route_mem, struct route_t, ROUTING_ROUTE_MAX_MEM);

uint8_t flood_sent_to_known(void);

/**
* \brief called by agent at startup
*/
void flood_init(void)
{
	PRINTF("FLOOD: init flood_route\n");
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

uint8_t flood_sent_to_known(void)
{
	struct discovery_neighbour_list_entry *nei_list = DISCOVERY.neighbours();
	struct discovery_neighbour_list_entry *nei_l;
	struct file_list_entry_t * pack = NULL;

	if( flood_transmitting ) {
		if( !flood_agent_event_pending ) {
			flood_agent_event_pending = 1;
			// Tell the agent to call us again to resubmit bundles
			process_post(&agent_process, dtn_bundle_resubmission_event, NULL);
		}

		return 0;
	}

	PRINTF("FLOOD: send to known neighbours\n");

	/**
	 * First step: look, if we know the direct destination already
	 * If so, always use direct delivery, never send to another node
	 */
	for(pack = (struct file_list_entry_t *) BUNDLE_STORAGE.get_bundles();
			pack != NULL;
			pack = list_item_next(pack)) {

		if( pack->routing.action == 1 ) {
			continue;
		}

		// Who is the destination for this bundle?
		rimeaddr_t dest_node = convert_eid_to_rime(pack->dest);

		for(nei_l = nei_list; nei_l != NULL; nei_l = list_item_next(nei_l)) {
			if( rimeaddr_cmp(&nei_l->neighbour, &dest_node) ) {

				if( rimeaddr_cmp(&pack->routing.last_node, &nei_l->neighbour) && pack->routing.last_counter >= ROUTING_MAX_TRIES ) {
					// We should avoid resubmitting this bundle now, because it failed too many times already
					PRINTF("FLOOD: Not resubmitting bundle to peer %u.%u\n", nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);
					continue;
				}

				// We know the neighbour, send it directly
				PRINTF("FLOOD: send bundle %u with SeqNo %lu to %u:%u directly\n", pack->bundle_num, pack->time_stamp_seq, nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

				struct route_t * route = memb_alloc(&route_mem);

				if( route == NULL ) {
					printf("FLOOD: cannot allocate route MEMB\n");
					return 0;
				}

				rimeaddr_copy(&route->dest, &nei_l->neighbour);
				route->bundle_num = pack->bundle_num;
				pack->routing.action = 1;

				flood_transmitting = 1;
				agent_send_bundles(route);

				// Only one bundle at a time
				return 1;
			}
		}
	}

	/**
	 * If we do not happen to have the destination as neighbour,
	 * flood it to everyone
	 */
	for(nei_l = nei_list; nei_l != NULL; nei_l = list_item_next(nei_l)) {
		PRINTF("FLOOD: neighbour %u.%u\n", nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

		for(pack = (struct file_list_entry_t *) BUNDLE_STORAGE.get_bundles();
				pack != NULL;
				pack = list_item_next(pack)) {

			if( pack->routing.action == 1 ) {
				continue;
			}

			PRINTF("FLOOD: Bundle %u, SRC %lu, DEST %lu, MSRC %u.%u, SEQ %lu\n", pack->bundle_num,  pack->src, pack->dest, pack->msrc.u8[0], pack->msrc.u8[1], pack->time_stamp_seq);

			uint8_t i, sent = 0;

			rimeaddr_t source_node = convert_eid_to_rime(pack->src);
			if( rimeaddr_cmp(&nei_l->neighbour, &source_node) ) {
				PRINTF("FLOOD: not sending bundle to originator\n");
				sent = 1;
			}

			if( rimeaddr_cmp(&nei_l->neighbour, &pack->msrc) ) {
				PRINTF("FLOOD: not sending back to sender\n");
				sent = 1;
			}

			for (i = 0 ; i < ROUTING_NEI_MEM ; i++) {
				if ( rimeaddr_cmp(&pack->routing.dest[i], &nei_l->neighbour)){
					PRINTF("FLOOD: bundle %u already sent to node %u:%u!\n", pack->bundle_num, pack->routing.dest[i].u8[0], pack->routing.dest[i].u8[1]);
					sent=1;
				}
			}

			if( rimeaddr_cmp(&pack->routing.last_node, &nei_l->neighbour) && pack->routing.last_counter >= ROUTING_MAX_TRIES ) {
				// We should avoid resubmitting this bundle now, because it failed too many times already
				PRINTF("FLOOD: Not resubmitting bundle to peer %u.%u\n", nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);
				sent = 1;
			}

			if(!sent){
				PRINTF("FLOOD: send bundle %u with SeqNo %lu to %u:%u\n", pack->bundle_num, pack->time_stamp_seq, nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

				struct route_t * route = memb_alloc(&route_mem);

				if( route == NULL ) {
					printf("FLOOD: cannot allocate route MEMB\n");
					return 0;
				}

				rimeaddr_copy(&route->dest, &nei_l->neighbour);
				route->bundle_num = pack->bundle_num;
				pack->routing.action=1;

				flood_transmitting = 1;
				agent_send_bundles(route);

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
void flood_resubmit_bundles(uint16_t bundle_num) {
	if( bundle_num == 1 ) {
		flood_agent_event_pending = 0;
	}

	flood_sent_to_known();
}

/**
* \brief addes a new bundle to the list of bundles
* \param bundle_num bundle number of the bundle
* \return >0 on success, <0 on error
*/
int flood_new_bundle(uint16_t bundle_num)
{
	statistics_bundle_incoming(1);

	return 1;
}

/**
* \brief deletes bundle from list
* \param bundle_num bundle nuber of the bundle
*/
void flood_del_bundle(uint16_t bundle_num)
{
	return;
}

/**
* \brief callback function sets the status of a bundle in the list
* \param route pointer to route struct 
* \param status status code
* \num_tx number of retransmissions 
*/
void flood_sent(struct route_t *route, int status, int num_tx)
{
	int post_resubmission_event_to_agent = 1;
	flood_transmitting = 0;

	struct file_list_entry_t * pack = NULL;
	for(pack = (struct file_list_entry_t *) BUNDLE_STORAGE.get_bundles();
			pack != NULL;
			pack = list_item_next(pack)) {

		if( pack->bundle_num == route->bundle_num ) {
			break;
		}
	}

	if( pack == NULL ) {
		PRINTF("FLOOD: Bundle %u not found\n", route->bundle_num );
		memb_free(&route_mem, route);
		return;
	}

	pack->routing.action = 0;

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

		if( rimeaddr_cmp(&pack->routing.last_node, &route->dest) ) {
			pack->routing.last_counter ++;

			if( pack->routing.last_counter >= ROUTING_MAX_TRIES ) {
				// We should avoid resubmitting this bundle now, because it failed too many times already
				PRINTF("FLOOD: Not resubmitting bundle to peer %u.%u\n", route->dest.u8[0], route->dest.u8[1]);

				// Notify discovery, that this peer apparently disappeared
				DISCOVERY.dead(&route->dest);

				// Reset our internal blacklist
				rimeaddr_copy(&pack->routing.last_node, &rimeaddr_null);
				pack->routing.last_counter = 0;

				post_resubmission_event_to_agent = 0;

				// BREAK! ;)
				break;
			}
		} else {
			rimeaddr_copy(&pack->routing.last_node, &route->dest);
			pack->routing.last_counter = 0;
		}

		break;

	case MAC_TX_OK:
		PRINTF("FLOOD: sent after %d tx\n", num_tx);
		statistics_bundle_outgoing(1);

		rimeaddr_copy(&pack->routing.last_node, &route->dest);
		pack->routing.last_counter = 0;

		if (pack->routing.send_to < ROUTING_NEI_MEM) {
			rimeaddr_copy(&pack->routing.dest[pack->routing.send_to], &route->dest);
			pack->routing.send_to++;

			PRINTF("FLOOD: bundle %u sent to %u nodes\n", route->bundle_num, pack->routing.send_to);

			rimeaddr_t dest_n = convert_eid_to_rime(pack->dest);
			if (rimeaddr_cmp(&route->dest, &dest_n)) {
				PRINTF("FLOOD: bundle sent to destination node, deleting bundle\n");
				agent_del_bundle(route->bundle_num);
				BUNDLE_STORAGE.del_bundle(route->bundle_num, REASON_NO_INFORMATION);
			} else {
				PRINTF("FLOOD: bundle for %u:%u delivered to %u:%u\n",dest_n.u8[0], dest_n.u8[1], route->dest.u8[0], route->dest.u8[1]);
			}
	    } else if (pack->routing.send_to >= ROUTING_NEI_MEM) {
	    	// Here we can delete the bundle from storage, because it will not be routed anyway
			PRINTF("FLOOD: bundle sent to max number of nodes, deleting bundle\n");
			agent_del_bundle(route->bundle_num);
			BUNDLE_STORAGE.del_bundle(route->bundle_num, REASON_NO_ROUTE);
		}

	    break;

	default:
	    printf("FLOOD: error %d after %d tx\n", status, num_tx);
	    break;
	}

	memb_free(&route_mem, route);

	// Tell the agent to call us again to resubmit bundles
	if( post_resubmission_event_to_agent && !flood_agent_event_pending ) {
		flood_agent_event_pending = 1;
		process_post(&agent_process, dtn_bundle_resubmission_event, NULL);
	}
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
};

