/**
 * \addtogroup routing
 * @{
 */

/**
 * \defgroup routing_null A very stupid routing module that primarily saves flash
 *
 * @{
 */

/**
 * \file 
 * \brief implementation of null routing
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <string.h>

#include "net/netstack.h"
#include "net/linkaddr.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "contiki.h"
#include "sys/clock.h"
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

//PROCESS(routing_process, "null routing process");
static TaskHandle_t routing_task;
static void routing_process(void* p);


bool routing_null_init(void)
{
	// Start Routing process
//	process_start(&routing_process, NULL);

	if ( !xTaskCreate(routing_process, "null routing process", configMINIMAL_STACK_SIZE, NULL, 1, &routing_task) ) {
		return false;
	}

	return true;
}

int routing_null_send_bundle(uint32_t bundle_number, linkaddr_t * neighbour)
{
	struct transmit_ticket_t * ticket = NULL;

	/* Allocate a transmission ticket */
	ticket = convergence_layer_get_transmit_ticket();
	if( ticket == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_WRN, "unable to allocate transmit ticket");
		return -1;
	}

	/* Specify which bundle */
	linkaddr_copy(&ticket->neighbour, neighbour);
	ticket->bundle_number = bundle_number;

	/* Put the bundle in the queue */
	convergence_layer_enqueue_bundle(ticket);

	return 1;
}

void routing_null_new_neighbour(linkaddr_t *dest)
{
	struct storage_entry_t * entry = NULL;

	/* Send all known bundles to that neighbour */
	for( entry = BUNDLE_STORAGE.get_bundles();
			entry != NULL;
			entry = list_item_next(entry) ) {
		/* Queue bundle for transmission */
		routing_null_send_bundle(entry->bundle_num, dest);
	}
}

void routing_null_send_to_known_neighbours(void)
{
	struct discovery_neighbour_list_entry *nei_l = NULL;
	struct storage_entry_t * entry = NULL;

	/* Iterate over all neighbours */
	for( nei_l = DISCOVERY.neighbours();
		 nei_l != NULL;
		 nei_l = list_item_next(nei_l) ) {

		/* Send all known bundles to that neighbour */
		for( entry = BUNDLE_STORAGE.get_bundles();
				entry != NULL;
				entry = list_item_next(entry) ) {

			/* Queue bundle for transmission */
			routing_null_send_bundle(entry->bundle_num, &nei_l->neighbour);
		}
	}

}

void routing_null_resubmit_bundles() {
//	process_poll(&routing_process);
	vTaskResume(routing_task);
}

int routing_null_new_bundle(uint32_t * bundle_number)
{
	struct mmem * bundlemem = NULL;
	struct bundle_t * bundle = NULL;
	struct discovery_neighbour_list_entry *nei_l = NULL;

	// Now go and request the bundle from storage
	bundlemem = BUNDLE_STORAGE.read_bundle(*bundle_number);
	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "unable to read bundle %lu", *bundle_number);
		return -1;
	}

	// Get our bundle struct and check the pointer
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_ROUTE, LOGL_ERR, "invalid bundle pointer for bundle %lu", *bundle_number);
		bundle_decrement(bundlemem);
		return -1;
	}

	/* Deliver the bundle - this will deallocate the memory */
	delivery_deliver_bundle(bundlemem);

	/* Iterate over all neighbours */
	for( nei_l = DISCOVERY.neighbours();
		 nei_l != NULL;
		 nei_l = list_item_next(nei_l) ) {

		/* And queue bundle for sending */
		routing_null_send_bundle(*bundle_number, &nei_l->neighbour);
	}

	// We do not have a failure here, so it must be a success
	return 1;
}

void routing_null_delete_bundle(uint32_t bundle_number)
{
	// This function is called by the agent if a bundle is deleted
	// We do not keep records, nothing to do here
}

void routing_null_bundle_sent(struct transmit_ticket_t * ticket, uint8_t status)
{
	if( status == ROUTING_STATUS_OK ) {
		/* Bundle has been forwarded at least once, delete it */
		BUNDLE_STORAGE.del_bundle(ticket->bundle_number, REASON_DELIVERED);
	}

	/* Free up the ticket */
	convergence_layer_free_transmit_ticket(ticket);

	/* Notify the process */
//	process_poll(&routing_process);
	vTaskResume(routing_task);
}

void routing_null_bundle_delivered_locally(struct mmem * bundlemem)
{
	struct bundle_t * bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	/* Delete bundle as it has been delivered at least once */
	if( bundle != NULL ) {
		BUNDLE_STORAGE.del_bundle(bundle->bundle_num, REASON_DELIVERED);
	}

	// Unblock the receiving service
	delivery_unblock_service(bundlemem);

	// Free the bundle memory
	bundle_decrement(bundlemem);

	/* Notify the process */
//	process_poll(&routing_process);
	vTaskResume(routing_task);
}

void routing_process(void* p)
{
//	PROCESS_BEGIN();

	LOG(LOGD_DTN, LOG_ROUTE, LOGL_INF, "null routing process in running");

	while(1) {
//		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
		vTaskSuspend(NULL);

		routing_null_send_to_known_neighbours();
	}

//	PROCESS_END();
}

const struct routing_driver routing_null ={
	"null_route",
	routing_null_init,
	routing_null_new_neighbour,
	routing_null_new_bundle,
	routing_null_delete_bundle,
	routing_null_bundle_sent,
	routing_null_resubmit_bundles,
	routing_null_bundle_delivered_locally,
};

