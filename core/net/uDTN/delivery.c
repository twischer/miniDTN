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
#include "dtn_config.h"
#include "status-report.h"
#include "sdnv.h"
#include "process.h"
#include "agent.h"
#include "custody.h"
#include "redundance.h"
#include "dev/leds.h"
#include "statistics.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


void deliver_bundle(struct bundle_t *bundle, struct registration *n) {

	PRINTF("DELIVERY\n");
	if(n->status == APP_ACTIVE) {
		PRINTF("DELIVERY: Service is active\n");

		if (!REDUNDANCE.check(bundle)) { //Bundle was not delivered before
			REDUNDANCE.set(bundle);
			statistics_bundle_delivered(1);
			process_post(n->application_process, submit_data_to_application_event, bundle);
			if (bundle->flags & BUNDLE_FLAG_CUST_REQ) {
				CUSTODY.report(bundle,128);
			}
		} else {
			delete_bundle(bundle);
		}
	}

	#if DEBUG_H
	uint16_t time = clock_time();
	time -= bundle->debug_time;
	PRINTF("DELIVERY: time needed to process bundle for Delivery: %i \n", time);
	#endif
}
/** @} */
