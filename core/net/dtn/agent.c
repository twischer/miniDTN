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

#include "net/dtn/API_registration.h"
#include "net/dtn/registration.h"
#include "net/dtn/API_events.h"
#include "net/dtn/bundle.h"
#include "net/dtn/agent.h"
#include "net/dtn/dtn_config.h"
#include "net/dtn/storage.h"
#include "net/dtn/sdnv.h"
#include "net/dtn/redundance.h"


#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint32_t dtn_node_id;

/* Makro das den Prozess definiert */
PROCESS(agent_process, "AGENT process");
AUTOSTART_PROCESSES(&agent_process);

/**
*   \brief Initialisierung des Bundle Protocols
*
*    Wird aus der main-Datei aufgerufen
*
*/
void agent_init(void) {
		
	PRINTF("starting DTN Bundle Protocol \n");
	process_start(&agent_process, NULL);
	BUNDLE_STORAGE.init();
//	BUNDLE_STORAGE.reinit();
	REDUNDANCE.init();
	dtn_node_id=15; //TODO was dynamisches

	
	
	dtn_application_registration_event = process_alloc_event();
	dtn_application_remove_event  = process_alloc_event();
	dtn_application_status_event = process_alloc_event();
	dtn_receive_bundle_event = process_alloc_event();
	dtn_send_bundle_event = process_alloc_event();
	submit_data_to_application_event = process_alloc_event();
	dtn_send_admin_record_event = process_alloc_event();
}


/* der Bundle Protocol Prozess */
PROCESS_THREAD(agent_process, ev, data)
{
	PROCESS_BEGIN();
	
	registration_init();
	//custody_init();
		
	struct bundle_t *bundleptr;
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
			time = time - bundleptr->rec_time;
			uint32_t lifetime;
			sdnv_decode(bundleptr->block + bundleptr->offset_tab[LIFE_TIME][OFFSET],sdnv_len(bundleptr->block + bundleptr->offset_tab[LIFE_TIME][OFFSET]),&lifetime);
			lifetime= lifetime -time;
			set_attr(bundleptr,LIFE_TIME,&lifetime);

			
//			while(bundlebuf_in_use())
//				PROCESS_PAUSE();
			
			//forwarding_bundle(bundleptr);
			continue;
		}
		
		else if(ev == dtn_receive_bundle_event) {
			
			PRINTF("BUNDLEPROTOCOL: bundle received \n");	
			bundleptr->rec_time= clock_time();
			
//			while(bundlebuf_in_use())
//				PROCESS_PAUSE();
			bundleptr= (struct bundle_t *) data;
			dispatch_bundle(bundleptr);
			//BUNDLE_STORAGE.save_bundle(bundleptr);
			delete_bundle(bundleptr);
			continue;
		}
		
		else if(ev == dtn_send_admin_record_event) {
			
			//reception_set_time();
			PRINTF("BUNDLEPROTOCOL: send admin record \n");
			bundleptr = (struct bundle_t *) data;
			
//			while(bundlebuf_in_use())
//				PROCESS_PAUSE();
			
			//forwarding_bundle(bundleptr);
			delete_bundle(bundleptr);
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
