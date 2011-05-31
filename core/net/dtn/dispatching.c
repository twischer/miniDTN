/**
 * \addtogroup dispatching
 * @{
 */

/**
 * \file
 *         Abfertigung des Bündels
 *
 *	Überprüfung des Ziels
 *
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "lib/list.h"
#include "net/dtn/agent.h"
#include "net/dtn/bundle.h"
#include "net/dtn/registration.h"
#include "net/dtn/sdnv.h"
//#include "expiration.h"
//#include "forwarding.h"
#include "net/dtn/administrative_record.h"
#include "net/dtn/custody.h"
#include "net/dtn/dtn_config.h"
#include "net/dtn/delivery.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


void dispatch_bundle(struct bundle_t *bundle) {
	
	PRINTF("DISPATCHING: bundle: %p\n", bundle->block);
	struct registration *n;

	uint32_t dest=1,flags=1;
	sdnv_decode(bundle->block + bundle->offset_tab[DEST_NODE][OFFSET], bundle->offset_tab[DEST_NODE][STATE], &dest);
	sdnv_decode(bundle->block + bundle->offset_tab[FLAGS][OFFSET], bundle->offset_tab[FLAGS][STATE], &flags);
 	//TODO warum sagt gcc das flags nicht bunutzt wird	
	uint32_t dest_app=1;
	sdnv_decode(bundle->block + bundle->offset_tab[DEST_SERV][OFFSET], bundle->offset_tab[DEST_SERV][STATE], &dest_app);


#if DEBUG
	PRINTF("DISPATCHING: [DEST_NODE][OFFSET]:%u [DEST_SERV][OFFSET]: %u \n", bundle->offset_tab[DEST_NODE][OFFSET],bundle->offset_tab[DEST_SERV][OFFSET]); 
	PRINTF("DISPATCHING: [DEST_NODE][STATE]:%u [DEST_SERV][STATE]: %u \n", bundle->offset_tab[DEST_NODE][STATE],bundle->offset_tab[DEST_SERV][STATE]); 
	PRINTF("DISPATCHING: destination eid: %lu:%lu \n", dest, dest_app);
	uint8_t i;
	for (i=0; i<17; i++){
//		PRINTF("DISPATCHING: offset: %u , len: %u\n", bundle->offset_tab[i][OFFSET],bundle->offset_tab[i][STATE]);
	}
#endif
	
	
	
	if((flags & 0x02) != 0) { //is bundle an admin record
		
		PRINTF("DISPATCHING: admin record detected \n");
		
		if(dest == dtn_node_id){
				
				administrative_record_block_t *admin_record;
				admin_record = (administrative_record_block_t *) bundle->block + bundle->offset_tab[DATA][OFFSET];
				
				if(admin_record->record_status == CUSTODY_SIGNAL) {
					PRINTF("DISPATCHING: received custody signal\n");	
					//call custody signal method
					CUSTODY.set_state(&admin_record->custody_signal);
				}
				return;
			}
	}
	
	else {
	
		uint32_t dest_app;
		sdnv_decode(bundle->block + bundle->offset_tab[DEST_SERV][OFFSET], bundle->offset_tab[DEST_SERV][STATE], &dest_app);

		PRINTF("DISPATCHING: destination eid: %lu:%lu  == %u, reg_list=%p\n", dest, dest_app,dtn_node_id,list_head(reg_list));
		
		if(dest == (uint32_t)dtn_node_id){
			for(n = list_head(reg_list); n != NULL; n = list_item_next(n)) {
				PRINTF("DISPATCHING: %lu == %lu\n", n->app_id, dest_app);	
				if(n->app_id == dest_app) {
				
					PRINTF("DISPATCHING: Registration found \n");
					deliver_bundle(bundle,n);
					return;
				}
			}
			PRINTF("DISPATCHING: no service registrated for bundel\n");
			return;
		}
	}
				
	forwarding_bundle(bundle);
	PRINTF("DISPATCHING: Bundle forwarded\n");

}
/** @} */
