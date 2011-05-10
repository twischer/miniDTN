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
#include "expiration.h"
#include "forwarding.h"
#include "administrative_record.h"
#include "net/dtn/custody.h"
#include "net/dtn/dtn_config.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


void dispatch_bundle(bundle_t *bundle) {
	
	struct registration *n;
	uint8_t nr_scheme, nr_ssp;
	
	uint32_t dest,flags;
	sdnv_decode(bundle->block + bundle->offset_tab[DEST_NODE][OFFSET], bundle->offset_tab[DEST_NODE][STATE], &dest)
	sdnv_decode(bundle->block + bundle->offset_tab[FLAGS][OFFSET], bundle->offset_tab[FLAGS][STATE], &flags)
	
	if((flags & 0x02) != 0) { //is bundle an admin record
		
		PRINTF("DISPATCHING: admin record detected \n");
		
		if(dest == dtn_node_id){
				
				administrative_record_block_t *admin_record;
				admin_record = (administrative_record_block_t *) bundle->_blockt+bundle->offset_tab[PAYLOAD][OFFSET];
				
				if(admin_record->record_status == CUSTODY_SIGNAL) {
					
					//call custody signal method
					CUSTODY.set_state(&admin_record->custody_signal);
				}
				return;
			}
	}
	
	else {
	
		uint32_t dest_app;
		sdnv_decode(bundle->block + bundle->offset_tab[DEST_APP][OFFSET], bundle->offset_tab[DEST_APP][STATE], &dest_app)

		PRINTF("DISPATCHING: destination eid: %lu:%lu \n", dest, dest_app);
		
		if(dest == dtn_node_id){
			for(n = list_head(reg_list); n != NULL; n = list_item_next(n)) {
			
				if(n->app_id == dest_app) {
				
					PRINTF("DISPATCHING: Registration found \n");
				
					deliver_bundle(bundle);
					return;
				}
			}
		}
	}
				
	forwarding_bundle(bundle);
	PRINTF("DISPATCHING: Bundle forwarded\n");

}
/** @} */
