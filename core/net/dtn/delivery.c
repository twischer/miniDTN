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
		process_post(n->application_process, submit_data_to_application_event, bundle);
		block = bundle->mem.ptr+1;
		if (*block & 0x08){
			custody_signal_t cust_sig;	
			cust_sig.status= CUSTODY_TRANSFER_SUCCEEDED;
			uint32_t flags;
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[FLAGS][OFFSET],bundle->offset_tab[FLAGS][STATE],&flags);
			if (flags & 0x01){ //bundle is fragmented
				sdnv_decode(bundle->mem.ptr + bundle->offset_tab[FRAG_OFFSET][OFFSET], bundle->offset_tab[FRAG_OFFSET][STATE], &cust_sig.fragement_offset);
				cust_sig.fragment_length=len;
			}else{
				cust_sig.fragment_length=0;
				cust_sig.fragement_offset=0;
			}
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[TIME_STAMP][OFFSET], bundle->offset_tab[TIME_STAMP][STATE],  &cust_sig.bundle_creation_timestamp);
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET],bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE],  &cust_sig.bundle_creation_timestamp_seq);
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[SRC_NODE][OFFSET], bundle->offset_tab[SRC_NODE][STATE], &cust_sig.src_node);
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[SRC_SERV][OFFSET], bundle->offset_tab[SRC_SERV][STATE], &cust_sig.src_app);

			CUSTODY.set_state(&cust_sig);
		}
	}			
	
	#if DEBUG_H
	uint16_t time = clock_time();
	time -= bundle->debug_time;
	PRINTF("DELIVERY: time needed to process bundle for Delivery: %i \n", time);
	#endif
}
/** @} */
