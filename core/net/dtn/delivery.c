/**
 * \addtogroup delivery
 * @{
 */

/**
 * \file
 *         Übergabefunktion an Anwendungen
 *
 */
 
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "net/dtn/custody-signal.h"

#include "lib/list.h"
#include "net/dtn/bundle.h"
#include "net/dtn/API_events.h"
#include "net/dtn/registration.h"
#include "net/dtn/dtn_config.h"
#include "net/dtn/status-report.h"
#include "net/dtn/sdnv.h"
#include "process.h"
#include "net/dtn/agent.h"
#include "net/dtn/custody.h"
#include "net/dtn/redundance.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


void deliver_bundle(struct bundle_t *bundle, struct registration *n) {

	PRINTF("DELIVERY\n");
	if(n->status == APP_ACTIVE) {  //TODO was passiert wenn eine applikation nicht aktiv ist
	PRINTF("DELIVERY: Service is active\n");

	

		uint32_t len;
		uint8_t *block = bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET];
#if DEBUG
		uint8_t block_count=0;
#endif
/*	
		while ( block <= bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE]){
			if (*block != 1){ // no payloadblock
				PRINTF("DELIVERY: block %u is no payload block\n", block_count); 
				uint8_t s_len= sdnv_len(block+2);
				sdnv_decode(block+2, s_len, &len);
				block= block +2 + len +s_len;
			}else{
				uint8_t s_len= sdnv_len(block+2);
				sdnv_decode(block+2, s_len, &len);
				PRINTF("DELIVERY: block %u is a payload block\n", block_count); 
				if( !REDUNDANCE.check(bundle)){ //packet was not delivert befor
					PRINTF("DELIVERY: bundle was not delivered befor\n");
					REDUNDANCE.set(bundle);
					process_post(n->application_process, submit_data_to_application_event, block +2 +s_len);
				}
				break;
			}
#if DEBUG
			block_count++;
#endif
		}
*/		
		if( !REDUNDANCE.check(bundle)){ //packet was not delivert befor
			printf("DELIVERY: bundle was not delivered befor\n");
			REDUNDANCE.set(bundle);
			process_post(n->application_process, submit_data_to_application_event, bundle);
			block = bundle->mem.ptr+1;
			if (*block & 0x08){
				CUSTODY.report(bundle,128);
			}
		}else{
			delete_bundle(bundle);
			printf("DELIVERY: muliple delivery\n");
		}
	}			
	
	#if DEBUG_H
	uint16_t time = clock_time();
	time -= bundle->debug_time;
	PRINTF("DELIVERY: time needed to process bundle for Delivery: %i \n", time);
	#endif
}
/** @} */
