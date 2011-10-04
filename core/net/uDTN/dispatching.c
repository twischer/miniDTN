/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "lib/list.h"
#include "agent.h"
#include "bundle.h"
#include "registration.h"
#include "sdnv.h"
#include "administrative_record.h"
#include "custody.h"
#include "dtn_config.h"
#include "delivery.h"
#include "status-report.h"
#include "forwarding.c"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


void dispatch_bundle(struct bundle_t *bundle) {
	PRINTF("DISPATCHING: bundle: %p\n", bundle->mem.ptr);
	struct registration *n;

	uint32_t dest=1;
	sdnv_decode(bundle->mem.ptr + bundle->offset_tab[DEST_NODE][OFFSET], bundle->offset_tab[DEST_NODE][STATE], &dest);
//	sdnv_decode(bundle->mem.ptr + bundle->offset_tab[FLAGS][OFFSET], bundle->offset_tab[FLAGS][STATE], &flags);
 	//TODO warum sagt gcc das flags nicht bunutzt wird	
	uint32_t dest_app=1;
	sdnv_decode(bundle->mem.ptr + bundle->offset_tab[DEST_SERV][OFFSET], bundle->offset_tab[DEST_SERV][STATE], &dest_app);


#if DEBUG
//	PRINTF("DISPATCHING: [DEST_NODE][OFFSET]:%u [DEST_SERV][OFFSET]: %u \n", bundle->offset_tab[DEST_NODE][OFFSET],bundle->offset_tab[DEST_SERV][OFFSET]); 
//	PRINTF("DISPATCHING: [DEST_NODE][STATE]:%u [DEST_SERV][STATE]: %u \n", bundle->offset_tab[DEST_NODE][STATE],bundle->offset_tab[DEST_SERV][STATE]); 
//	PRINTF("DISPATCHING: destination eid: %lu:%lu \n", dest, dest_app);
//	uint8_t i;
//	for (i=0; i<17; i++){
//		PRINTF("DISPATCHING: offset: %u , len: %u\n", bundle->offset_tab[i][OFFSET],bundle->offset_tab[i][STATE]);
//	}
#endif
	
	
	
	if((bundle->flags & 0x02) != 0) { //is bundle an admin record
		
		//printf("DISPATCHING: admin record detected \n");
		
		if(dest == dtn_node_id){
				//printf("its for me\n");		
				
				if(*((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]) & 32 ) {// is custody signal
					PRINTF("DISPATCHING: received custody signal %u %u\n",bundle->offset_tab[DATA][OFFSET], bundle->mem.size);	
				#if DEBUG
					uint8_t i=0;
					for (i=0; i< bundle->mem.size-bundle->offset_tab[DATA][OFFSET]; i++){
						PRINTF("%x:", *((uint8_t*)bundle->mem.ptr +i+ bundle->offset_tab[DATA][OFFSET]));
					}
					PRINTF("\n");
				#endif
					//call custody signal method
					if (*((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1) & 128 ){	
						CUSTODY.release(bundle);
					}else{
						CUSTODY.retransmit(bundle);
					}

				}else if( (*((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]) & 16) && (*((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1) & 2)) { // node accepted custody
				//printf(" node acced\n");
				#if DEBUG
					uint8_t i=0;
					for (i=0; i< bundle->mem.size; i++){
						PRINTF("%x:", *((uint8_t*)bundle->mem.ptr +i));
					}
					PRINTF("\n");
				#endif
					

					CUSTODY.release(bundle);
				}
				delete_bundle(bundle);
				return;
		}
	}
	
	else {
	
					//leds_on(1);
		uint32_t dest_app;
		sdnv_decode(bundle->mem.ptr + bundle->offset_tab[DEST_SERV][OFFSET], bundle->offset_tab[DEST_SERV][STATE], &dest_app);

		PRINTF("DISPATCHING: destination eid: %lu:%lu  == %lu, reg_list=%p\n", dest, dest_app,dtn_node_id,list_head(reg_list));
		//if (bundle->flags & 0x08){ // bundle is custody
		//	STATUS_REPORT.send(bundle, 2,0);
		//}
		if(dest == (uint32_t)dtn_node_id){
			for(n = list_head(reg_list); n != NULL; n = list_item_next(n)) {
				PRINTF("DISPATCHING: %lu == %lu\n", n->app_id, dest_app);	
				if(n->app_id == dest_app) {
					PRINTF("DISPATCHING: Registration found \n");
					deliver_bundle(bundle,n);
					//leds_off(1);	
					return;
				}
			}
			PRINTF("DISPATCHING: no service registrated for bundel\n");
			delete_bundle(bundle);
			return;
		}
	}
				
	forwarding_bundle(bundle);
	PRINTF("DISPATCHING: Bundle forwarded\n");

}
/** @} */
