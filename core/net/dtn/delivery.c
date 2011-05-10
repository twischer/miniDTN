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
		

			uint32_t len;
			uint8_t *block = bundle->block + bundle->offset_tab[DATA][OFFSET];
			while ( block <= bundle->block + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE]){
				if (*block != 1){ // no payloadblock
					uint8_t s_len= sdnv_len(bock+2);
					sdnv_decode(block+2, s_len, &len);
					block= block +2 + len +s_len;
				}else{
					uint8_t s_len= sdnv_len(bock+2);
					sdnv_decode(block+2, s_len, &len);
					process_post(n->application_process, submit_data_to_application_event, block +2 +s_len);
					break;
				}
			}
			block = bundle->block+1;
			if (*block & 0x08){
				custody_signal_t cust_sig;	
				cust_sig.status= CUSTODY_TRANSFER_SUCCEEDED;
				uint8_t *tmp= bundle->block + bundle->offset_tab[DATA][OFFSET]+1; 
				uint32_t flags;
				sdnv_decode(bundle->block + bundle->offset_tab[FLAGS][OFFSET],bundle->offset_tab[FLAGS][STATE],&flags);
				if (flags & 0x01){ //bundle is fragmented
					sdnv_decode(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET], bundle->offset_tab[FRAG_OFFSET][STATE], &cust_sig.fragement_offset);
					cust_sig.fragment_length=len;
				}else{
					cust_sig.fragment_length=0;
					cust_sig.fragement_offset=0;
				}
				sdnv_decode(bundle->block + bundle->offset_tab[TIME_STAMP][OFFSET], bundle->offset_tab[TIME_STAMP][STATE],  &cust_sig.bundle_creation_timestamp);
				sdnv_decode(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET],bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE],  &cust_sig.bundle_creation_timestamp_seq);
				sdnv_decode(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET], bundle->offset_tab[SRC_NODE][STATE], &cust_sig.src_node);
				sdnv_decode(bundle->block + bundle->offset_tab[SRC_SERV][OFFSET], bundle->offset_tab[SRC_SERV][STATE], &cust_sig.src_app);

				CUSTODY.set_state(&cust_sig);
			}
		}			
	}
	
	
	#if DEBUG
	time = clock_time();
	time -= bundle->rec_time;
	PRINTF("DELIVERY: time needed to process bundle for Delivery: %i \n", time);
	#endif
}
/** @} */
