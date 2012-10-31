/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 *
 */
 
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "custody-signal.h"

#include "lib/list.h"
#include "bundle.h"
#include "API_events.h"
#include "registration.h"
#include "status-report.h"
#include "sdnv.h"
#include "process.h"
#include "agent.h"
#include "custody.h"
#include "delivery.h"
#include "redundance.h"
#include "dev/leds.h"
#include "statistics.h"
//#define ENABLE_LOGGING 1
#include "logging.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void unblock_service(struct mmem * bundlemem) {
	struct registration * n = NULL;
	struct bundle_t * bundle = NULL;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "invalid MMEM pointer");
		return;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "invalid bundle");
		bundle_dec(bundlemem);
		return;
	}

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
int deliver_bundle(struct mmem *bundlemem) {
	struct registration * n = NULL;
	struct bundle_t * bundle = NULL;
	int delivered = 0;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "invalid MMEM pointer");
		return -1;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "invalid bundle");
		bundle_dec(bundlemem);
		return -1;
	}

	// Check if the bundle has been delivered before
	if( REDUNDANCE.check(bundlemem) ) {
		bundle_dec(bundlemem);
		return -1;
	}

	// Let's see, what service registrations we have
	for(n = list_head(reg_list);
		n != NULL;
		n = list_item_next(n)) {

		if(n->app_id == bundle->dst_srv) {
			if(n->status == APP_ACTIVE) {
				if( !n->busy ) {
					LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "Service found, delivering...");

					// Mark service as busy to prevent further bundle deliveries
					n->busy = 1;

					// Post the event to the respective service
					process_post(n->application_process, submit_data_to_application_event, bundlemem);

					delivered = 1;

					// We deliver only to the first service in line - multiple registrations are not supported
					break;
				} else
					LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "Service is busy");
			} else
				LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "Service is inactive");
		}
	}

	if( !delivered ) {
		// if we did not find a registration, deallocate the memory
		bundle_dec(bundlemem);

		// Return error code
		return -1;
	}

	// Notify statistics
	statistics_bundle_delivered(1);

	// Put the bundle into the list of already delivered bundles
	REDUNDANCE.set(bundlemem);

	// And report to custody
	if (bundle->flags & BUNDLE_FLAG_CUST_REQ) {
		CUSTODY.report(bundlemem, 128);
	}

#if DEBUG_H
	uint16_t time = clock_time();
	time -= bundle->debug_time;
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "time needed to process bundle for Delivery: %i", time);
#endif

	return 1;
}
/** @} */
