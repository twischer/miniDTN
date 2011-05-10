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


#include "lib/list.h"
#include "API/bundle.h"
#include "API/DTN-block-types.h"
#include "API/primary_block.h"
#include "API/API_events.h"
#include "registration.h"
#include "dtn-config.h"
#include "deletion.h"
#include "send-record.h"
#include "status-report.h"
#include "process.h"
#include "bundle-protocol.h"
#include "reception.h"
#include "dictionary.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


void deliver_bundle(struct bundle_t *bundle, struct registration *n) {
	
	uint8_t i = 0;
	uint8_t nr_scheme, nr_ssp;
	int32_t time;
		if(n->status == APP_ACTIVE) {
		

			uint8_t *block = bundle->block + bundle->offset_tab[DATA][OFFSET];
			while ( block <= bundle->block + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE]){
				if (*block != 1){ // no payloadblock
					uint32_t len;
					uint8_t s_len= sdnv_len(bock+2);
					sdnv_decode(block+2, s_len, &len);
					block= block +2 + len +s_len;
				}else{
					uint8_t s_len= sdnv_len(bock+2);
					process_post(n->application_process, submit_data_to_application_event, block +2 +s_len);
					break;
				}
			}
			block = bundle->block+1;
			if (*block & 0x08){
				CUSTODY.set_state(//TODO custody dings erzeugen


			/* Übergebe Nutzdaten jedes enthaltenen Payload Blocks an Anwendung */
			while(bundle->bundle_payload_block[i].block_type == 1) {
				
				
				if(n->status)
					process_post(n->application_process, submit_data_to_application_event, bundle->bundle_payload_block[i].block_data);
				if((bundle->bundle_payload_block[i].payload_pcf & 0x08) != 0)
					break;
				i++;		
			}
			
			/* Wenn Custody Transfer Flag gesetzt, sende Custody Signal */
			if((bundle->bundle_primary_block.primary_pcf & 0x08) != 0) {

				send_custody_signal(bundle, CUSTODY_TRANSFER_SUCCEEDED, NO_ADDITIONAL_INFORMATION);
			}
		}			
	}
	
	free(eid);
	
	#if DEBUG
	time = clock_time();
	time -= reception_get_time();
	PRINTF("DELIVERY: time needed to process bundle for Delivery: %i \n", time);
	#endif
}
/** @} */
