/**
 * \addtogroup routing
 * @{
 */

/**
 * \defgroup routing_flooding Flooding Routing module
 *
 * @{
 */

/**
 * \file 
 * \brief implementation of flood routing
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <string.h>

#include "net/netstack.h"
#include "net/linkaddr.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/logging.h"

#include "bundle.h"
#include "storage.h"
#include "sdnv.h"
#include "agent.h"
#include "discovery.h"
#include "statistics.h"
#include "bundleslot.h"
#include "delivery.h"
#include "convergence_layer.h"
#include "registration.h"

#include "routing.h"

#define BLACKLIST_TIMEOUT	10
#define BLACKLIST_THRESHOLD	3
#define BLACKLIST_SIZE		3

/**
 * Internally used return values
 */
#define FLOOD_ROUTE_RETURN_OK 1
#define FLOOD_ROUTE_RETURN_CONTINUE 0
#define FLOOD_ROUTE_RETURN_FAIL -1

struct blacklist_entry_t {
	struct blacklist_entry_t * next;

	cl_addr_t node;
	uint8_t counter;
	TickType_t timestamp;
};

struct routing_list_entry_t {
	/** pointer to the next entry */
	struct routing_list_entry_t * next;

	/** pointer to MMEM containing the routing_entry_t */
	struct mmem entry;
} __attribute__ ((packed));

struct routing_entry_t {
	/** number of the bundle */
	uint32_t bundle_number;

	/** bundle flags */
	uint8_t flags;

	/** number of nodes the bundle has been sent to already */
	uint8_t send_to;

	/** addresses of nodes this bundle was sent to (is the EID of the nodes) */
	// TODO Use uint32_t to support bigger EIDs for UDP-CL
	linkaddr_t neighbours[ROUTING_NEI_MEM];

	/** bundle destination */
	uint32_t destination_node;

	/** bundle source */
	uint32_t source_node;

	/** neighbour from which we have received the bundle */
	// TODO should be the EID,
	// because the message should not send over another CL to the same node
	cl_addr_t received_from_node;
} __attribute__ ((packed));

/**
 * Routing process
 */
static TaskHandle_t routing_task;
static void routing_process(void* p);

/* only used to produce a logging output after BLACKLIST_THRESHOLD */
MEMB(blacklist_mem, struct blacklist_entry_t, BLACKLIST_SIZE);
LIST(blacklist_list);

MEMB(routing_mem, struct routing_list_entry_t, BUNDLE_STORAGE_SIZE);
LIST(routing_list);

void routing_flooding_send_to_known_neighbours(void);
void routing_flooding_check_keep_bundle(uint32_t bundle_number);


/**
 * @brief routing_flooding_neighbour_to_addr creates an unified address
 * from a neighbour entry.
 * An IP address is higher prior.
 * So always an IP address is available,
 * The IP addresse and the port will be used.
 * @param entry
 * @param addr
 */
static int routing_flooding_neighbour_to_addr(const struct discovery_neighbour_list_entry* const entry, cl_addr_t* const addr)
{
	if (discovery_neighbour_to_addr(entry, ADDRESS_TYPE_FLAG_IPV4, addr) >= 0) {
		/* ip address successful converted */
		return 0;
	} else if (discovery_neighbour_to_addr(entry, ADDRESS_TYPE_FLAG_LOWPAN, addr) >= 0) {
		/* lowpan address successful converted */
		return 0;
	} else {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "Could not find a valid address in discovery list entry for %u.%u",
			entry->neighbour.u8[0], entry->neighbour.u8[1]);
		return -1;
	}
}


/**
 * \brief Adds (or refreshes) the entry of 'neighbour' on the blacklist
 * \param neighbour Address of the neighbour
 * \return 1 if neighbour is (now) blacklisted, 0 otherwise
 */
static int routing_flooding_blacklist_add(const cl_addr_t* const neighbour)
{
	struct blacklist_entry_t * entry;

	for(entry = list_head(blacklist_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		if( cl_addr_cmp(neighbour, &entry->node) ) {
			if( (xTaskGetTickCount() - entry->timestamp) > pdMS_TO_TICKS(BLACKLIST_TIMEOUT * 1000) ) {
				// Reusing existing (timedout) entry
				entry->counter = 0;
			}

			entry->counter ++;
			entry->timestamp = xTaskGetTickCount();

			if( entry->counter > BLACKLIST_THRESHOLD ) {
				char addr_str[CL_ADDR_STRING_LENGTH];
				cl_addr_string(neighbour, addr_str, sizeof(addr_str));
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "%s blacklisted", addr_str);
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

	cl_addr_copy(&entry->node, neighbour);
	entry->counter = 1;
	entry->timestamp = xTaskGetTickCount();

	list_add(blacklist_list, entry);

	return 0;
}

/**
 * \brief Deletes a neighbour from the blacklist
 * \param neighbour Address of the neighbour
 */
static void routing_flooding_blacklist_delete(const cl_addr_t* const neighbour)
{
	struct blacklist_entry_t * entry;

	for(entry = list_head(blacklist_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		if( cl_addr_cmp(neighbour, &entry->node) ) {
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
bool routing_flooding_init(void)
{
	// Start CL process
	if ( !xTaskCreate(routing_process, "FLOOD ROUTE process", 0x100, NULL, 1, &routing_task) ) {
		return false;
	}

	return true;
}

/**
 * \brief Poll our process, so that we can resubmit bundles
 */
void routing_flooding_schedule_resubmission(void)
{
//	process_poll(&routing_process);
	vTaskResume(routing_task);
}

/**
 * \brief checks if there are bundle to send to dest
 * \param dest pointer to the address of the new neighbor
 */
void routing_flooding_new_neighbour(linkaddr_t *dest)
{
	routing_flooding_schedule_resubmission();
}

/**
 * \brief Send bundle to neighbour
 * \param bundle_number Number of the bundle
 * \param neighbour Address of the neighbour
 * \return 1 on success, -1 on error
 */
static int routing_flooding_send_bundle(uint32_t bundle_number, const cl_addr_t* const neighbour)
{
	struct transmit_ticket_t * ticket = NULL;

	/* Allocate a transmission ticket */
	ticket = convergence_layer_get_transmit_ticket();
	if( ticket == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_WRN, "unable to allocate transmit ticket");
		return -1;
	}

	/* Specify which bundle */
	cl_addr_copy(&ticket->neighbour, neighbour);
	ticket->bundle_number = bundle_number;

	/* Put the bundle in the queue */
	convergence_layer_enqueue_bundle(ticket);

	return 1;
}

/**
 * \brief Deliver a bundle to a local service
 * \param entry Pointer to the routing entry of the bundle
 * \return FLOOD_ROUTE_RETURN_OK if delivered, FLOOD_ROUTE_RETURN_CONTINUE otherwise
 */
int routing_flooding_send_to_local(struct routing_entry_t * entry)
{
	struct mmem * bundlemem = NULL;
	int ret = 0;

	// Should this bundle be delivered locally?
	if( !(entry->flags & ROUTING_FLAG_LOCAL) || (entry->flags & ROUTING_FLAG_IN_DELIVERY) ) {
		return FLOOD_ROUTE_RETURN_CONTINUE;
	}

	bundlemem = BUNDLE_STORAGE.read_bundle(entry->bundle_number);
	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "cannot read bundle %lu", entry->bundle_number);
		return FLOOD_ROUTE_RETURN_CONTINUE;
	}

	ret = delivery_deliver_bundle(bundlemem);
	if( ret == DELIVERY_STATE_WAIT_FOR_APP ) {
		entry->flags |= ROUTING_FLAG_IN_DELIVERY;
		return FLOOD_ROUTE_RETURN_OK;
	} else if( ret == DELIVERY_STATE_DELETE ) {
		// Bundle can be deleted right away
		entry->flags &= ~ROUTING_FLAG_LOCAL;

		// Reschedule ourselves
		routing_flooding_schedule_resubmission();

		// And remove bundle if applicable
		routing_flooding_check_keep_bundle(entry->bundle_number);
	} else if( ret == DELIVERY_STATE_BUSY ) {
		return FLOOD_ROUTE_RETURN_OK;
	}

	return FLOOD_ROUTE_RETURN_CONTINUE;
}


/**
 * \brief Forward a bundle to its destination
 * \param entry Pointer to the routing entry of the bundle
 * \return FLOOD_ROUTE_RETURN_OK if queued, FLOOD_ROUTE_RETURN_CONTINUE if not queued and FLOOD_ROUTE_RETURN_FAIL of queue is full
 */
int routing_flooding_forward_directly(struct routing_entry_t * entry)
{
	struct discovery_neighbour_list_entry *nei_l = NULL;
	linkaddr_t dest_node;
	int h = 0;

	/* Who is the destination for this bundle? */
	dest_node = convert_eid_to_rime(entry->destination_node);

	/* First step: check, if the destination is one of our neighbours */
	for( nei_l = DISCOVERY.neighbours();
		 nei_l != NULL;
		 nei_l = list_item_next(nei_l) ) {

		if( linkaddr_cmp(&nei_l->neighbour, &dest_node) ) {
			break;
		}
	}

	if( nei_l == NULL ) {
		return FLOOD_ROUTE_RETURN_CONTINUE;
	}

	/* We know the neighbour, send it directly */
	LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "send bundle %lu to %u.%u directly", entry->bundle_number, nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

	/* Mark bundle as busy */
	entry->flags |= ROUTING_FLAG_IN_TRANSIT;

	/* use ip addresse and port for destination, if existing */
	cl_addr_t neighbour;
	if (routing_flooding_neighbour_to_addr(nei_l, &neighbour) < 0) {
		/*
		 * the neighbour address could not be extracted,
		 * so try to flood the bundle to all nieghbours
		 */
		return FLOOD_ROUTE_RETURN_CONTINUE;
	}

	/* And queue it for sending */
	h = routing_flooding_send_bundle(entry->bundle_number, &neighbour);
	if( h < 0 ) {
		/* Enqueuing bundle failed - unblock it */
		entry->flags &= ~ROUTING_FLAG_IN_TRANSIT;

		/* If sending the bundle fails, all other will likely also fail */
		return FLOOD_ROUTE_RETURN_FAIL;
	}

	/* We do not want the bundle to be sent to anybody else at the moment, so: */
	return FLOOD_ROUTE_RETURN_OK;
}

/**
 * \brief Forward a bundle to the next hop
 * \param entry Pointer to the routing entry of the bundle
 * \return FLOOD_ROUTE_RETURN_OK if queued, FLOOD_ROUTE_RETURN_CONTINUE if not queued and FLOOD_ROUTE_RETURN_FAIL of queue is full
 */
int routing_flooding_forward_normal(struct routing_entry_t * entry)
{
	struct discovery_neighbour_list_entry *nei_l = NULL;
	linkaddr_t source_node;
	int h = 0;

	/* What is the source node of the bundle? */
	source_node = convert_eid_to_rime(entry->source_node);

	for( nei_l = DISCOVERY.neighbours();
		 nei_l != NULL;
		 nei_l = list_item_next(nei_l) ) {
		int sent = 0;
		int i;

		if( linkaddr_cmp(&nei_l->neighbour, &source_node) ) {
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "not sending bundle to originator");

			/* Go on with the next neighbour */
			continue;
		}

		/* create a corresponding cl_addr for the neighbour entry */
		cl_addr_t neighbour;
		if (routing_flooding_neighbour_to_addr(nei_l, &neighbour) < 0) {
			/*
			 * the neighbour address could not be extracted,
			 * so continue with the next list entry
			 */
			continue;
		}

		if( cl_addr_cmp(&neighbour, &entry->received_from_node) ) {
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "not sending back to sender");

			/* Go on with the next neighbour */
			continue;
		}

		/* Did we forward the bundle to that neighbour already? */
		for (i = 0 ; i < ROUTING_NEI_MEM ; i++) {
			if ( linkaddr_cmp(&entry->neighbours[i], &nei_l->neighbour)){
				LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %lu already sent to node %u.%u!", entry->bundle_number, entry->neighbours[i].u8[0], entry->neighbours[i].u8[1]);
				sent = 1;

				// Break the (narrowest) for
				break;
			}
		}

		if(!sent) {
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "send bundle %lu to %u.%u", entry->bundle_number, nei_l->neighbour.u8[0], nei_l->neighbour.u8[1]);

			/* Mark bundle as busy */
			entry->flags |= ROUTING_FLAG_IN_TRANSIT;

			/* And queue it for sending */
			h = routing_flooding_send_bundle(entry->bundle_number, &neighbour);
			if( h < 0 ) {
				/* Enqueuing bundle failed - unblock it */
				entry->flags &= ~ROUTING_FLAG_IN_TRANSIT;

				/* If sending the bundle fails, all other will likely also fail */
				return FLOOD_ROUTE_RETURN_FAIL;
			}

			/* Only one bundle at a time */
			return FLOOD_ROUTE_RETURN_OK;
		}
	}

	return FLOOD_ROUTE_RETURN_CONTINUE;
}

/**
 * \brief iterate through all bundles and forward bundles
 */
void routing_flooding_send_to_known_neighbours(void)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;
	int try_to_forward = 1;
	int try_local = 1;
	int h = 0;

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

		if( try_local ) {
			/* Is the bundle for local? */
			h = routing_flooding_send_to_local(entry);

			/* We can only deliver only bundle at a time to local processes to speed up the whole thing */
			if( h == FLOOD_ROUTE_RETURN_OK ) {
				try_local = 0;
			}
		}

		/* Skip this bundle, if it is not queued for forwarding */
		if( !(entry->flags & ROUTING_FLAG_FORWARD) || (entry->flags & ROUTING_FLAG_IN_TRANSIT) || !try_to_forward ) {
			continue;
		}

		/* Try to forward it to the destination, if it is our neighbour */
		h = routing_flooding_forward_directly(entry);
		if( h == FLOOD_ROUTE_RETURN_OK ) {
			/* Bundle will be delivered, to skip the remainder if this function*/
			continue;
		} else if( h == FLOOD_ROUTE_RETURN_CONTINUE ) {
			/* Bundle was not delivered, continue as normal */
		} else if( h == FLOOD_ROUTE_RETURN_FAIL ) {
			/* Enqueuing the bundle failed, to stop the forwarding process */
			try_to_forward = 0;
			continue;
		}

		/* At this point, we know that the bundle is not for one of our neighbours, so send it to all the others */
		h = routing_flooding_forward_normal(entry);
		if( h == FLOOD_ROUTE_RETURN_OK ) {
			/* Bundle will be forwarded, continue as normal */
		} else if( h == FLOOD_ROUTE_RETURN_CONTINUE ) {
			/* Bundle will not be forwarded, continue as normal */
		} else if( h == FLOOD_ROUTE_RETURN_FAIL ) {
			/* Enqueuing the bundle failed, to stop the forwarding process */
			try_to_forward = 0;
			continue;
		}
	}
}

/**
 * \brief Wrapper function for agent calls to resubmit bundles for already known neighbours
 */
void routing_flooding_resubmit_bundles() {
	routing_flooding_schedule_resubmission();
}

/**
 * \brief Checks whether a bundle still has to be kept or can be deleted
 * \param bundle_number Number of the bundle
 */
void routing_flooding_check_keep_bundle(uint32_t bundle_number) {
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

	if( n == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "Bundle not in storage yet");
		return;
	}

	if( (entry->flags & ROUTING_FLAG_LOCAL) || (entry->flags & ROUTING_FLAG_FORWARD) ) {
		return;
	}

	LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "Deleting bundle %lu", bundle_number);
	BUNDLE_STORAGE.del_bundle(bundle_number, REASON_DELIVERED);
}

/**
 * \brief Adds a new bundle to the list of bundles
 * \param bundle_number bundle number of the bundle
 * \return >0 on success, <0 on error
 */
int routing_flooding_new_bundle(const uint32_t bundle_number)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;
	struct mmem * bundlemem = NULL;
	struct bundle_t * bundle = NULL;

	LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "agent announces bundle %lu", bundle_number);

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
		bundle_decrement(bundlemem);
		return -1;
	}

	// Now we have our entry
	// We have to get the pointer AFTER getting the bundle from storage, because accessing the
	// storage may change the MMEM structure and thus the pointers!
	entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);
	memset(entry, 0, sizeof(struct routing_entry_t));

	// Nothing can go wrong anymore, add the (surrounding) struct to the list
	list_add(routing_list, n);

	/* Here we decide if a bundle is to be delivered locally and/or forwarded */
	if( bundle->dst_node == dtn_node_id ) {
		/* This bundle is for our node_id, deliver locally */
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle is for local");
		entry->flags |= ROUTING_FLAG_LOCAL;
	} else {
		/* This bundle is not (directly) for us and will be forwarded */
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle is for forward");
		entry->flags |= ROUTING_FLAG_FORWARD;
	}

	if( !(bundle->flags & BUNDLE_FLAG_SINGLETON) ) {
		/* Bundle is not Singleton, so forward it in any case */
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle is for forward");
		entry->flags |= ROUTING_FLAG_FORWARD;
	}

	if( registration_is_local(bundle->dst_srv, bundle->dst_node) && bundle->dst_node != dtn_node_id) {
		/* Bundle is for a local registration, so deliver it locally */
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle is for local and forward");
		entry->flags |= ROUTING_FLAG_LOCAL;
		entry->flags |= ROUTING_FLAG_FORWARD;
	}

	// Now copy the necessary attributes from the bundle
	entry->bundle_number = bundle_number;
	bundle_get_attr(bundlemem, DEST_NODE, &entry->destination_node);
	bundle_get_attr(bundlemem, SRC_NODE, &entry->source_node);
	cl_addr_copy(&entry->received_from_node, &bundle->msrc);

	// Now that we have the bundle, we do not need the allocated memory anymore
	bundle_decrement(bundlemem);
	bundlemem = NULL;
	bundle = NULL;

	// Schedule to deliver and forward the bundle
	routing_flooding_schedule_resubmission();

	// We do not have a failure here, so it must be a success
	return 1;
}

/**
 * \brief deletes bundle from list
 * \param bundle_number bundle number of the bundle
 */
void routing_flooding_delete_bundle(uint32_t bundle_number)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;

	LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "flood_del_bundle for bundle %lu", bundle_number);

	// Find the bundle in our internal storage
	for( n = list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry->bundle_number == bundle_number ) {
			break;
		}
	}

	if( n == NULL ) {
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


uint32_t routing_get_eid_of_cl_addr(const cl_addr_t* const addr)
{
	if (addr->isIP) {
		struct discovery_neighbour_list_entry* nei_l = DISCOVERY.neighbours();
		for(; nei_l != NULL; nei_l = list_item_next(nei_l) ) {
			cl_addr_t nei_addr;
			if (discovery_neighbour_to_addr(nei_l, ADDRESS_TYPE_FLAG_IPV4, &nei_addr) < 0) {
				/* convertion of ip address failed, possibly this entry does not contain an IP */
				continue;
			}

			if (cl_addr_cmp(addr, &nei_addr)) {
				break;
			}
		}

		if (nei_l == NULL) {
			return 0;
		}

		return convert_rime_to_eid(&nei_l->neighbour);
	} else {
		// TODO remove const cast
		return convert_rime_to_eid( (linkaddr_t*)&addr->lowpan );
	}
}


/**
 * \brief Callback function informing us about the status of a sent bundle
 * \param ticket CL transmit ticket of the bundle
 * \param status status code
 */
void routing_flooding_bundle_sent(struct transmit_ticket_t * ticket, uint8_t status)
{
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;

	// Tell the agent to call us again to resubmit bundles
	routing_flooding_schedule_resubmission();

	// Find the bundle in our internal storage
	for( n = list_head(routing_list);
		 n != NULL;
		 n = list_item_next(n) ) {

		entry = (struct routing_entry_t *) MMEM_PTR(&n->entry);

		if( entry->bundle_number == ticket->bundle_number ) {
			break;
		}
	}

	if( n == NULL ) {
		/* Free up the ticket */
		convergence_layer_free_transmit_ticket(ticket);

		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "Bundle not in storage");
		return;
	}

	/* Bundle is not busy anymore */
	entry->flags &= ~ROUTING_FLAG_IN_TRANSIT;

	if( status == ROUTING_STATUS_FAIL ||
		status == ROUTING_STATUS_TEMP_NACK ) {
		// temporary NACK = Other side rejected the bundle, try again later
		// FAIL = Transmission failed
		// --> note down address in blacklist
		routing_flooding_blacklist_add(&ticket->neighbour);

		/* Free up the ticket */
		convergence_layer_free_transmit_ticket(ticket);

		return;
	}

	if( status == ROUTING_STATUS_ERROR ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "Bundle %lu has fatal error, deleting", ticket->bundle_number);

		/* Bundle failed permanently, we can delete it because it will never be delivered anyway */
		entry->flags = 0;

		routing_flooding_check_keep_bundle(ticket->bundle_number);

		/* Free up the ticket */
		convergence_layer_free_transmit_ticket(ticket);

		return;
	}

	// Here: status == ROUTING_STATUS_OK
	// Or:   status == ROUTING_STATUS_NACK (which we handle as an ACK to avoid sending the bundle again)
	statistics_bundle_outgoing(1);

	routing_flooding_blacklist_delete(&ticket->neighbour);

#ifndef TEST_DO_NOT_DELETE_ON_DIRECT_DELIVERY
	const uint32_t eid = routing_get_eid_of_cl_addr(&ticket->neighbour);
	if (entry->destination_node == eid && status != ROUTING_STATUS_NACK) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle sent to destination node");
		const uint32_t bundle_number = ticket->bundle_number;

		/* Free up the ticket */
		convergence_layer_free_transmit_ticket(ticket);
		ticket = NULL;

		// Unset the forward flag
		entry->flags &= ~ROUTING_FLAG_FORWARD;
		routing_flooding_check_keep_bundle(bundle_number);

		return;
	}

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(&ticket->neighbour, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle for ipn:%lu delivered to %s", entry->destination_node, addr_str);
#endif


	if (entry->send_to < ROUTING_NEI_MEM) {
		const uint32_t eid = routing_get_eid_of_cl_addr(&ticket->neighbour);
		if (eid <= 0) {
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_WRN, "Could not find EID for bundle %lu", ticket->bundle_number);
		} else {
			// TODO use eid in neighbour list
			const linkaddr_t lowpan = convert_eid_to_rime(eid);
			linkaddr_copy(&entry->neighbours[entry->send_to], &lowpan);
			entry->send_to++;
			LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %lu sent to %u nodes", ticket->bundle_number, entry->send_to);
		}
	} else if (entry->send_to >= ROUTING_NEI_MEM) {
		// Here we can delete the bundle from storage, because it will not be routed anyway
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %lu sent to max number of nodes, deleting", ticket->bundle_number);

		// Unset the forward flag
		entry->flags &= ~ROUTING_FLAG_FORWARD;
		routing_flooding_check_keep_bundle(ticket->bundle_number);
	}

	/* Free up the ticket */
	convergence_layer_free_transmit_ticket(ticket);
}

/**
 * \brief Incoming notification, that service has finished processing bundle
 * \param bundlemem Pointer to the MMEM struct of the bundle
 */
void routing_flooding_bundle_delivered_locally(struct mmem * bundlemem) {
	struct routing_list_entry_t * n = NULL;
	struct routing_entry_t * entry = NULL;
	struct bundle_t * bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	// Tell the agent to call us again to resubmit bundles
	routing_flooding_schedule_resubmission();

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
	delivery_unblock_service(bundlemem);

	// Free the bundle memory
	bundle_decrement(bundlemem);

	/* We count ourselves as node as well, so list us as receiver of a bundle copy */
	if (entry->send_to < ROUTING_NEI_MEM) {
		linkaddr_copy(&entry->neighbours[entry->send_to], &linkaddr_node_addr);
		entry->send_to++;
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %lu sent to %u nodes", entry->bundle_number, entry->send_to);
	} else if (entry->send_to >= ROUTING_NEI_MEM) {
		// Here we can delete the bundle from storage, because it will not be routed anyway
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_DBG, "bundle %lu sent to max number of nodes, deleting", entry->bundle_number);

		/* Unsetting the forward flag will make routing_flooding_check_keep_bundle delete the bundle */
		entry->flags &= ~ROUTING_FLAG_FORWARD;
	}

	// Check remaining live of bundle
	routing_flooding_check_keep_bundle(entry->bundle_number);
}

/**
 * \brief Routing persistent process
 */
void routing_process(void* p)
{
	LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "FLOOD ROUTE process in running");

	// Initialize memory used to store blacklisted neighbours
	memb_init(&blacklist_mem);
	list_init(blacklist_list);

	// Initialize memory used to store bundles for routing
	memb_init(&routing_mem);
	list_init(routing_list);

	while(1) {
		vTaskSuspend(NULL);

		routing_flooding_send_to_known_neighbours();
	}
}

const struct routing_driver routing_flooding ={
	"flood_route",
	routing_flooding_init,
	routing_flooding_new_neighbour,
	routing_flooding_new_bundle,
	routing_flooding_delete_bundle,
	routing_flooding_bundle_sent,
	routing_flooding_resubmit_bundles,
	routing_flooding_bundle_delivered_locally,
};

