/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 *        
 */
 
#include "cfs.h"
#include "cfs-coffee.h"

#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "timer.h"
#include "net/netstack.h"
#include "mmem.h"
#include "net/rime/rimeaddr.h"

#include "API_registration.h"
#include "registration.h"
#include "API_events.h"
#include "bundle.h"
#include "agent.h"
#include "storage.h"
#include "sdnv.h"
#include "redundance.h"
#include "dispatching.h"
#include "routing.h"
#include "dtn-network.h"
#include "node-id.h"
#include "custody.h"
#include "status-report.h"
#include "lib/memb.h"
#include "discovery.h"
#include "statistics.h"

// #define ENABLE_LOGGING 1
#include "logging.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static struct mmem * bundleptr;

uint32_t dtn_node_id;
uint32_t dtn_seq_nr;
PROCESS(agent_process, "AGENT process");
AUTOSTART_PROCESSES(&agent_process);
struct etimer resubmission_timer;

void agent_init(void) {
	process_start(&agent_process, NULL);
}

/** FIXME: We need a return value here to indicate whether we have really told the MAC to do the transmission.
 * Otherwise the callback will not come and the routing is locked forever!
 */
void
agent_send_bundles(struct route_t * route)
{
	struct mmem * bundlemem = NULL;
	struct bundle_t *bundle = NULL;

	// Here we check, if we have a local bundle
	bundlemem = BUNDLE_STORAGE.read_bundle(route->bundle_num);
	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "unable to read bundle %d", route->bundle_num);
		return;
	}

	// Get our bundle struct and check the pointer
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "invalid bundle pointer for bundle %d", route->bundle_num);
		bundle_dec(bundlemem);
		return;
	}

	// How long did this bundle rot in our storage?
	uint32_t elapsed_time = clock_seconds() - bundle->rec_time;

	// Check if bundle has expired
	if( bundle->lifetime < elapsed_time ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "bundle %d has expired, not sending it", route->bundle_num);

		// Bundle is expired
		bundle_dec(bundlemem);

		BUNDLE_STORAGE.del_bundle(route->bundle_num, REASON_LIFETIME_EXPIRED);
		return;
	}

	// Update remaining lifetime of bundle
	uint32_t remaining_time = bundle->lifetime - elapsed_time;
	set_attr(bundlemem, LIFE_TIME, &remaining_time);

	// And send it out
	dtn_network_send(bundlemem, route);

	// Now deallocate the memory
	bundle_dec(bundlemem);
}

/*  Bundle Protocol Prozess */
PROCESS_THREAD(agent_process, ev, data)
{
	PROCESS_BEGIN();
	
	
	mmem_init();
	BUNDLE_STORAGE.init();
	BUNDLE_STORAGE.reinit();
	ROUTING.init();
	REDUNDANCE.init();
	dtn_node_id=node_id; 
	dtn_seq_nr=0;
	registration_init();
	
	dtn_application_remove_event  = process_alloc_event();
	dtn_application_registration_event = process_alloc_event();
	dtn_application_status_event = process_alloc_event();
	dtn_receive_bundle_event = process_alloc_event();
	dtn_send_bundle_event = process_alloc_event();
	submit_data_to_application_event = process_alloc_event();
	dtn_beacon_event = process_alloc_event();
	dtn_send_admin_record_event = process_alloc_event();
	dtn_bundle_in_storage_event = process_alloc_event();
	dtn_send_bundle_to_node_event = process_alloc_event();
	dtn_bundle_resubmission_event = process_alloc_event();
	dtn_processing_finished = process_alloc_event();
	dtn_bundle_stored = process_alloc_event();
	
	CUSTODY.init();
	DISCOVERY.init();
	PRINTF("starting DTN Bundle Protocol \n");
		

	etimer_set(&resubmission_timer, 5 * CLOCK_SECOND);

	struct registration_api *reg;
	
	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev);

		if(ev == dtn_application_registration_event) {
			reg = (struct registration_api *) data;
			registration_new_app(reg->app_id, reg->application_process);
			PRINTF("BUNDLEPROTOCOL: Event empfangen, Registration, Name: %lu\n", reg->app_id);
			continue;
		}
					
		if(ev == dtn_application_status_event) {

			int status;
			reg = (struct registration_api *) data;
			PRINTF("BUNDLEPROTOCOL: Event empfangen, Switch Status, Status: %i \n", reg->status);
			if(reg->status == APP_ACTIVE)
				status = registration_set_active(reg->app_id);
			else if(reg->status == APP_PASSIVE)
				status = registration_set_passive(reg->app_id);
			
			if(status == -1) {
				PRINTF("BUNDLEPROTOCOL: no registration found to switch \n");
			}

			continue;
		}
		
		if(ev == dtn_application_remove_event) {
			reg = (struct registration_api *) data;
			PRINTF("BUNDLEPROTOCOL: Event empfangen, Remove, Name: %lu \n", reg->app_id);
			registration_remove_app(reg->app_id);
			continue;
		}
		
		if(ev == dtn_send_bundle_event) {
			uint32_t * bundle_number;
			uint8_t n = 0;
			struct bundle_t * bundle = NULL;

			bundleptr = (struct mmem *) data;
			if( bundleptr == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_send_bundle_event with invalid pointer");
				continue;
			}

			bundle = (struct bundle_t *) MMEM_PTR(bundleptr);
			if( bundle == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_send_bundle_event with invalid MMEM structure");
				continue;
			}

			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_send_bundle_event(%p) with seqNo %lu", bundleptr, dtn_seq_nr);

			// Set the outgoing sequence number
			set_attr(bundleptr, TIME_STAMP_SEQ_NR, &dtn_seq_nr);
			dtn_seq_nr++;

			// Save the bundle in storage
			n = BUNDLE_STORAGE.save_bundle(bundleptr, &bundle_number);

			// If a sender process exists, notify it
			if( n && bundle->source_process != NULL) {
				// Bundle has been successfully saved, send event to service
				process_post(bundle->source_process, dtn_bundle_stored, bundleptr);
			}

			// Now emulate the event to our agent
			if( n ) {
				data = bundle_number;
				ev = dtn_bundle_in_storage_event;
				// process_post(&agent_process, dtn_bundle_in_storage_event, bundle_number);
			}
		}
		
		if(ev == dtn_send_admin_record_event) {
			PRINTF("BUNDLEPROTOCOL: send admin record \n");
			continue;
		}

		if(ev == dtn_beacon_event){
			rimeaddr_t* src =(rimeaddr_t*) data;
			ROUTING.new_neighbor(src);
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_beacon_event for %u.%u", src->u8[0], src->u8[1]);
			continue;
		}

		if(ev == dtn_bundle_in_storage_event){
			uint32_t bundle_number = *(uint32_t *) data;

			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "bundle %lu in storage", bundle_number);

			if(ROUTING.new_bundle(bundle_number) < 0){
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "routing reports error when announcing new bundle %lu", bundle_number);
				continue;
			}

			ROUTING.resubmit_bundles(0);
			continue;
		}
		
		if(ev == dtn_bundle_resubmission_event) {
			ROUTING.resubmit_bundles(1);

			etimer_restart(&resubmission_timer);

			continue;
		}

	    if(ev == dtn_processing_finished) {
	    	// data should contain the bundlemem ptr
	    	struct mmem * bundlemem = (struct mmem *) data;

	    	// Notify routing, that service has finished processing a bundle
	    	ROUTING.locally_delivered(bundlemem);
	    }

		if( etimer_expired(&resubmission_timer) ) {
			ROUTING.resubmit_bundles(0);

			// Even if there is nothing to do, call resubmit every second just to be absolutely sure
			etimer_reset(&resubmission_timer);

			continue;
		}
	}
	PROCESS_END();
}

void agent_del_bundle(uint32_t bundle_number){
	ROUTING.del_bundle(bundle_number);
	CUSTODY.del_from_list(bundle_number);
}
/** @} */
