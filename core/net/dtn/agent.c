/**
 * \addtogroup process
 * @{
 */

/**
 * \file
 *         Implementierung des Bundle Agent Process
 *
 */
 
#include "cfs.h"
#include "cfs-coffee.h"

#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "timer.h"
#include "net/rime/rimeaddr.h"

#include "net/dtn/API_registration.h"
#include "net/dtn/registration.h"
#include "net/dtn/API_events.h"
#include "net/dtn/bundle.h"
#include "net/dtn/agent.h"
#include "net/dtn/dtn_config.h"
#include "net/dtn/storage.h"
#include "net/dtn/sdnv.h"
#include "net/dtn/redundance.h"
#include "net/dtn/dispatching.h"
#include "net/dtn/forwarding.h"
#include "net/dtn/routing.h"
#include "net/dtn/dtn-network.h"
#include "node-id.h"


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint32_t dtn_node_id;
static uint32_t dtn_seq_nr;
static struct etimer discover_timer;
/* Makro das den Prozess definiert */
PROCESS(agent_process, "AGENT process");
AUTOSTART_PROCESSES(&agent_process);

/**
*   \brief Initialisierung des Bundle Protocols
*
*    Wird aus der main-Datei aufgerufen
*&agent_process
*/
void agent_init(void) {
		
	PRINTF("starting DTN Bundle Protocol \n");
	process_start(&agent_process, NULL);
	BUNDLE_STORAGE.init();
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
//	BUNDLE_STORAGE.reinit();

}


/* der Bundle Protocol Prozess */
PROCESS_THREAD(agent_process, ev, data)
{
	PROCESS_BEGIN();
	
	
	//custody_init();
		
	static struct bundle_t *bundleptr;
	struct registration_api *reg;
	
	while(1) {
		
		PROCESS_WAIT_EVENT_UNTIL(ev);
		if(ev == dtn_application_registration_event) {
			
			reg = (struct registration_api *) data;
			registration_new_app(reg->app_id, reg->application_process);
			PRINTF("BUNDLEPROTOCOL: Event empfangen, Registration, Name: %lu\n", reg->app_id);
			continue;
		}
					
		else if(ev == dtn_application_status_event) {

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
		
		else if(ev == dtn_application_remove_event) {
			
			reg = (struct registration_api *) data;
			PRINTF("BUNDLEPROTOCOL: Event empfangen, Remove, Name: %lu \n", reg->app_id);
			registration_remove_app(reg->app_id);
			continue;
		}
		
		else if(ev == dtn_send_bundle_event) {
			
			PRINTF("BUNDLEPROTOCOL: bundle send \n");
			//reception_set_time();
			bundleptr = (struct bundle_t *) data;
			uint32_t time=(uint32_t) clock_seconds();
			if (bundleptr->size >= 110){
				PRINTF("BUNDLEPROTOCOL: bundle too big size: %u\n" , bundleptr->size);
				continue;
			}
			//time = time - bundleptr->rec_time;
			//uint32_t lifetime;
			//sdnv_decode(bundleptr->block + bundleptr->offset_tab[LIFE_TIME][OFFSET],sdnv_len(bundleptr->block + bundleptr->offset_tab[LIFE_TIME][OFFSET]),&lifetime);
			//lifetime= lifetime -time;
			//set_attr(bundleptr,LIFE_TIME,&lifetime);
			set_attr(bundleptr,TIME_STAMP_SEQ_NR,&dtn_seq_nr);
			printf("BUNDLEPROTOCOL: seq_num = %lu\n",dtn_seq_nr);	
			dtn_seq_nr++;
//			while(bundlebuf_in_use())
//				PROCESS_PAUSE();
			
			forwarding_bundle(bundleptr);
			continue;
		}
		
		else if(ev == dtn_receive_bundle_event) {
			
			PRINTF("BUNDLEPROTOCOL: bundle received \n");	
			
//			while(bundlebuf_in_use())
//				PROCESS_PAUSE();
			bundleptr= (struct bundle_t *) data;
//			bundleptr->rec_time= (uint32_t) clock_seconds();


			dispatch_bundle(bundleptr);
			//BUNDLE_STORAGE.save_bundle(bundleptr);
			delete_bundle(bundleptr);
//			packetbuf_clear();
			PRINTF("BUNDLEPROTOCOL: end bundle received \n");
		//	*/
			continue;
		}
		
		else if(ev == dtn_send_admin_record_event) {
			
			//reception_set_time();
			PRINTF("BUNDLEPROTOCOL: send admin record \n");
		//	bundleptr = (struct bundle_t *) data;
			
//			while(bundlebuf_in_use())
//				PROCESS_PAUSE();
			
			//forwarding_bundle(bundleptr);
		//	delete_bundle(bundleptr);
			continue;
		}

		else if(ev == dtn_beacon_event){
			rimeaddr_t* src =(rimeaddr_t*) data;	
			PRINTF("BUNDLEPROTOCOL: got beacon from %u:%u\n",src->u8[1],src->u8[0]);
			ROUTING.new_neighbor(src);
			continue;
		}
		
		else if(ev == dtn_bundle_in_storage_event){
			PRINTF("BUNDLEPROTOCOL: bundle in storage\n");	
			uint16_t b_num= *(uint16_t *) data;
			ROUTING.new_bundle(b_num);
			dtn_discover();
			if (BUNDLE_STORAGE.get_bundle_num() == 1){
				etimer_set(&discover_timer, DISCOVER_CYCLE*CLOCK_SECOND);
			}
			continue;
		}
		
		else if(ev == dtn_bundle_deleted_event){
			uint16_t *tmp= (uint16_t *) data;
			PRINTF("BUNDLEPROTOCOL: delete bundle %u\n",*tmp);
			ROUTING.del_bundle( *tmp);
			//free(tmp);
			continue;
		}

		else if(ev == dtn_send_bundle_to_node_event){
			struct route_t *route = (struct route_t *)data;
			PRINTF("BUNDLEPROTOCOL: send bundle %u to node %u:%u\n",route->bundle_num, route->dest.u8[1], route->dest.u8[0]);
			BUNDLE_STORAGE.read_bundle(route->bundle_num,bundleptr);
			PRINTF("BUNDLEPROTOCOL: bundle ready\n");
			bundleptr->bundle_num =  route->bundle_num;
			uint32_t remaining_time= ((uint32_t) clock_seconds())-bundleptr->rec_time;
			set_attr(bundleptr,LIFE_TIME,&remaining_time);
			dtn_network_send(bundleptr,route);
			continue;
		}
		
		else if(etimer_expired(&discover_timer)){
			PRINTF("BUNDLEPROTOCOL: discover_timer\n");
			if (BUNDLE_STORAGE.get_bundle_num()>0){
				PRINTF("BUNDLEPROTOCOL: sending discover and reschedule timer to %u seconds %u bundles in storage\n",DISCOVER_CYCLE,BUNDLE_STORAGE.get_bundle_num());
				etimer_set(&discover_timer, DISCOVER_CYCLE*CLOCK_SECOND);
				dtn_discover();	
			}else{
				PRINTF("BUNDLEPROTOCOL: no more bundles to transmit\n");
			}
			continue;
		}
		/*
		else if(etimer_expired(&custody_etimer)) {
			PRINTF("BUNDLEPROTOCOL: Custody Timer expired \n");
			reception_set_time();

			while(bundlebuf_in_use())
				PROCESS_PAUSE();
			custody_read_bundle(&bundle);
			forwarding_bundle_from_custody(&bundle);
			delete_bundle(&bundle);
			continue;
		}
		*/
	}
	PROCESS_END();
}
/** @} */
