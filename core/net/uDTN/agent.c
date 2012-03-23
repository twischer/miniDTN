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


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint32_t dtn_node_id;
uint32_t dtn_seq_nr;
static struct etimer discover_timer;
PROCESS(agent_process, "AGENT process");
AUTOSTART_PROCESSES(&agent_process);

void agent_init(void) {
		
	process_start(&agent_process, NULL);
}

void
agent_send_bundles(struct route_t *route)
{
	struct bundle_t bundle;
	struct bundle_t *bundleptr;
	uint8_t listcount=0;

	while (route !=NULL) {
		listcount++;
		PRINTF("BUNDLEPROTOCOL: send bundle %u to node %u:%u\n",route->bundle_num, route->dest.u8[1], route->dest.u8[0]);
		memset(&bundle, 0, sizeof(struct bundle_t));
		bundleptr = &bundle;
		if(BUNDLE_STORAGE.read_bundle(route->bundle_num,bundleptr)<=0){
			route= route->next;
			return;
		}
		PRINTF("BUNDLEPROTOCOL: bundleptr->mem->ptr %p\n", bundleptr->mem.ptr);
		PRINTF("BUNDLEPROTOCOL: bundle ready\n");
		bundleptr->bundle_num =  route->bundle_num;
		uint32_t remaining_time= bundleptr->lifetime-(((uint32_t) clock_seconds())-bundleptr->rec_time);
		if (remaining_time <= bundleptr->lifetime) {
			PRINTF("BUNDLEPROTOCOL: %lu-%lu-%lu=%lu\n",bundleptr->lifetime, (uint32_t) clock_seconds(),bundleptr->rec_time,bundleptr->lifetime-(((uint32_t) clock_seconds())-bundleptr->rec_time));
			set_attr(bundleptr,LIFE_TIME,&remaining_time);
			PRINTF("BUNDLEPROTOCOL: bundleptr->mem->ptr %p %u\n", bundleptr->mem.ptr, bundleptr->mem.size);
			dtn_network_send(bundleptr,route);
			delete_bundle(bundleptr);
		}else{
			ROUTING.sent(route,MAC_TX_NOACK,0);
			uint16_t tmp=bundleptr->bundle_num;
			delete_bundle(bundleptr);
			BUNDLE_STORAGE.del_bundle(tmp,1);
		}
		route= route->next;
	}
	ROUTING.delete_list();
	return;;
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
	dtn_bundle_deleted_event = process_alloc_event();
	dtn_send_bundle_to_node_event = process_alloc_event();

	
	CUSTODY.init();
	PRINTF("starting DTN Bundle Protocol \n");
		
	static struct bundle_t * bundleptr;
	struct bundle_t bundle;

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
			
			#if DEBUG
			if(status == -1)
				PRINTF("BUNDLEPROTOCOL: no registration found to switch \n");
			#endif
			continue;
		}
		
		if(ev == dtn_application_remove_event) {
			
			reg = (struct registration_api *) data;
			PRINTF("BUNDLEPROTOCOL: Event empfangen, Remove, Name: %lu \n", reg->app_id);
			registration_remove_app(reg->app_id);
			continue;
		}
		
		if(ev == dtn_send_bundle_event) {
			PRINTF("BUNDLEPROTOCOL: bundle send \n");
			bundleptr = (struct bundle_t *) data;
			
			bundleptr->rec_time=(uint32_t) clock_seconds(); 
			if (bundleptr->size >= 110){
				PRINTF("BUNDLEPROTOCOL: bundle too big size: %u\n" , bundleptr->size);
				continue;
			}
			set_attr(bundleptr,TIME_STAMP_SEQ_NR,&dtn_seq_nr);
			PRINTF("\nBUNDLEPROTOCOL: seq_num = %lu\n",dtn_seq_nr);	
			dtn_seq_nr++;
				
			/* Fall through to dtn_bundle_in_storage_event if forwarding_bundle succeeded */
			data = forwarding_bundle(bundleptr);
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
			PRINTF("BUNDLEPROTOCOL: bundle in storage %u %p %p\n",b_num, data, saved_as_mem);	
			memb_free(saved_as_mem,data);
			if(ROUTING.new_bundle(b_num) < 0){
				PRINTF("BUNDLEPROTOCOL: ERROR\n");
				continue;
			}
			PRINTF("BUNDLEPROTOCOL: discover\n");
			DISCOVERY.send(b_num);
			if (BUNDLE_STORAGE.get_bundle_num() == 1){
				etimer_set(&discover_timer, DISCOVER_CYCLE*CLOCK_SECOND);
			}

			continue;
		}
		
		if(ev == dtn_bundle_deleted_event){
			ROUTING.del_bundle( del_num);
			CUSTODY.del_from_list(del_num);
			continue;
		}
		
		if(etimer_expired(&discover_timer)){
			PRINTF("BUNDLEPROTOCOL: discover_timer\n");
			if (BUNDLE_STORAGE.get_bundle_num()>0){
				PRINTF("BUNDLEPROTOCOL: sending discover and reschedule timer to %f seconds %u bundles in storage\n",DISCOVER_CYCLE,BUNDLE_STORAGE.get_bundle_num());
				etimer_set(&discover_timer, DISCOVER_CYCLE*CLOCK_SECOND);
				DISCOVERY.send(255);
			}else{
				PRINTF("BUNDLEPROTOCOL: no more bundles to transmit\n");
			}
			continue;
		}
	}
	PROCESS_END();
}
void agent_del_bundle(void){

			ROUTING.del_bundle( del_num);
			CUSTODY.del_from_list(del_num);
}
/** @} */
