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
#include "dtn_config.h"
#include "agent.h"
#include "storage.h"
#include "sdnv.h"
#include "redundance.h"
#include "dispatching.h"
#include "forwarding.h"
#include "routing.h"
#include "dtn-network.h"
#include "node-id.h"
#include "custody.h"
#include "status-report.h"
#include "lib/memb.h"
#include "discovery.h"
#include "statistics.h"

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

void
agent_send_bundles(struct route_t * route)
{
	struct mmem *bundlemem;
	struct bundle_t *bundle;

	bundlemem = BUNDLE_STORAGE.read_bundle(route->bundle_num);
	if (!bundlemem) {
		PRINTF("BUNDLEPROTOCOL: agent_send_bundles() cannot find bundle %u\n", route->bundle_num);
		return;
	}

	bundle = MMEM_PTR(bundlemem);

	// How long did this bundle rot in our storage?
	uint32_t elapsed_time = clock_seconds() - bundle->rec_time;

	// Do not send bundles that are expired
	if( bundle->lifetime < elapsed_time ) {
		PRINTF("BUNDLEPROTOCOL: bundle expired\n");

		// Bundle is expired
		uint16_t tmp = bundle->bundle_num;
		bundle_dec(bundle);
		BUNDLE_STORAGE.del_bundle(tmp, REASON_LIFETIME_EXPIRED);

		return;
	}

	// Bundle can still be sent
	uint32_t remaining_time = bundle->lifetime - elapsed_time;
	set_attr(bundlemem, LIFE_TIME, &remaining_time);
	dtn_network_send(bundlemem, route);

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
			struct bundle_t *bundle;
			PRINTF("BUNDLEPROTOCOL: bundle send \n");
			bundleptr = (struct mmem *) data;

			bundle = MMEM_PTR(bundleptr);
			bundle->rec_time=(uint32_t) clock_seconds(); 
			set_attr(bundleptr,TIME_STAMP_SEQ_NR,&dtn_seq_nr);
			PRINTF("BUNDLEPROTOCOL: seq_num = %lu\n",dtn_seq_nr);
			dtn_seq_nr++;

			// Notify statistics module
			statistics_bundle_generated(1);
				
			/* Fall through to dtn_bundle_in_storage_event if forwarding_bundle succeeded */
			data = (void *)forwarding_bundle(bundleptr);

			// make sure, that nobody overwrites our pointer
			bundleptr = NULL;

			if (data) {
				ev = dtn_bundle_in_storage_event;
			} else {
				continue;
			}
		}
		
		if(ev == dtn_send_admin_record_event) {
			PRINTF("BUNDLEPROTOCOL: send admin record \n");
			continue;
		}

		if(ev == dtn_beacon_event){
			rimeaddr_t* src =(rimeaddr_t*) data;
			ROUTING.new_neighbor(src);
			PRINTF("BUNDLEPROTOCOL: foooooo\n");
			continue;
		}

		if(ev == dtn_bundle_in_storage_event){
			uint16_t b_num = *(uint16_t *) data;
			memb_free(saved_as_mem, data);

			PRINTF("BUNDLEPROTOCOL: bundle in storage %u %p\n",b_num, data);
			if(ROUTING.new_bundle(b_num) < 0){
				PRINTF("BUNDLEPROTOCOL: ERROR\n");
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

		if( etimer_expired(&resubmission_timer) ) {
			ROUTING.resubmit_bundles(0);

			// Even if there is nothing to do, call resubmit every second just to be absolutely sure
			etimer_reset(&resubmission_timer);

			continue;
		}
	}
	PROCESS_END();
}

void agent_del_bundle(uint16_t bundle_number){
	ROUTING.del_bundle(bundle_number);
	CUSTODY.del_from_list(bundle_number);
}
/** @} */
