/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 *
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */
 
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/list.h"
#include "sys/process.h"
#include "lib/logging.h"

#include "bundle.h"
#include "api.h"
#include "dtn_process.h"
#include "registration.h"
#include "statusreport.h"
#include "sdnv.h"
#include "agent.h"
#include "custody.h"
#include "statistics.h"
#include "storage.h"

#include "delivery.h"

void delivery_unblock_service(struct mmem * bundlemem) {
	struct registration * n = NULL;
	struct bundle_t * bundle = NULL;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "invalid MMEM pointer");
		return;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "invalid bundle");
		bundle_decrement(bundlemem);
		return;
	}

	// Unlock the bundle so that it can be deleted
	BUNDLE_STORAGE.unlock_bundle(bundle->bundle_num);

	// Let's see, what service registrations we have
	for(n = list_head(reg_list);
		n != NULL;
		n = list_item_next(n)) {

		if(n->app_id == bundle->dst_srv) {
			if(n->status == APP_ACTIVE) {
				if( n->busy ) {
					LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "unblocking service");
					n->busy = 0;
					return;
				}
			}
		}
	}

	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "No Service found to unblock");
}

/**
 * \brief Delivers bundle to a local service
 * \param bundlemem Pointer to the MMEM bundle representation
 * \returns <0 on error >=0 on success
 */
int delivery_deliver_bundle(struct mmem *bundlemem) {
	struct registration * n = NULL;
	struct bundle_t * bundle = NULL;
	int delivered = 0;
	int busy = 0;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "invalid MMEM pointer");
		return DELIVERY_STATE_ERROR;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "invalid bundle");
		bundle_decrement(bundlemem);
		return DELIVERY_STATE_ERROR;
	}

	// Let's see, what service registrations we have
	for(n = list_head(reg_list);
		n != NULL;
		n = list_item_next(n)) {

		if(n->app_id == bundle->dst_srv &&
		   n->node_id == bundle->dst_node ) {
			if(n->status == APP_ACTIVE) {
				if( !n->busy ) {
					LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "Service found, delivering...");

					// Mark service as busy to prevent further bundle deliveries
					n->busy = 1;

					// Lock the bundle so it will not be deleted in the meantime
					BUNDLE_STORAGE.lock_bundle(bundle->bundle_num);

					// Post the event to the respective service
//					process_post(n->event_queue, submit_data_to_application_event, bundlemem);
					dtn_process_send_event(n->event_queue, submit_data_to_application_event, bundlemem);

					delivered = 1;

					// We deliver only to the first service in line - multiple registrations are not supported
					break;
				} else {
					LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "Service is busy");
					busy = 1;
				}
			} else
				LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "Service is inactive");
		}
	}

	if( !delivered && busy ) {
		// if we did not find a registration, deallocate the memory
		bundle_decrement(bundlemem);
		// Return error code
		return DELIVERY_STATE_BUSY;
	}

	if( !delivered ) {
		// if we did not find a registration, deallocate the memory
		bundle_decrement(bundlemem);

		// Return error code
		return DELIVERY_STATE_ERROR;
	}

	// Notify statistics
	statistics_bundle_delivered(1);

	// And report to custody
	if (bundle->flags & BUNDLE_FLAG_CUST_REQ) {
		CUSTODY.report(bundlemem, 128);
	}

	// Possibly send status report
	if( bundle->flags & BUNDLE_FLAG_REP_DELIV ) {
		STATUSREPORT.send(bundlemem, NODE_DELIVERED_BUNDLE, NO_ADDITIONAL_INFORMATION);
	}

	return DELIVERY_STATE_WAIT_FOR_APP;
}
/** @} */
