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
#include "forwarding.h"
//#define ENABLE_LOGGING 1
#include "logging.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


void dispatch_bundle(struct mmem *bundlemem) {
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	struct registration *n;
	uint16_t *saved_as_mem;

	if ((bundle->flags & BUNDLE_FLAG_ADM_REC)) { //is bundle an admin record
		//printf("DISPATCHING: admin record detected \n");
		
		if (bundle->dst_node == dtn_node_id) {
			//printf("its for me\n");

			/* XXX FIXME: Administrative records are not supported yet
			 *
			if(*(MMEM_PTR(bundle->block.type) & 32 ) {// is custody signal
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
			}*/
			delete_bundle(bundlemem);
			return;
		}
	} else {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "DISPATCHING: destination eid: %lu:%lu  == %lu, reg_list=%p", bundle->dst_node, bundle->dst_srv,dtn_node_id,list_head(reg_list));
		//if (bundle->flags & 0x08){ // bundle is custody
		//	STATUS_REPORT.send(bundle, 2,0);
		//}

		if(bundle->dst_node == (uint32_t)dtn_node_id){
			for(n = list_head(reg_list); n != NULL; n = list_item_next(n)) {
				PRINTF("DISPATCHING: %lu == %lu\n", n->app_id, bundle->dst_srv);
				if(n->app_id == bundle->dst_srv) {
					LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "DISPATCHING: Registration found");
					deliver_bundle(bundlemem,n);
					return;
				}
			}
			LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "DISPATCHING: no service registered for bundle");
			delete_bundle(bundlemem);
			return;
		}
	}

	saved_as_mem = forwarding_bundle(bundlemem);
	if (saved_as_mem) {
		process_post(&agent_process, dtn_bundle_in_storage_event, saved_as_mem);
	}

	PRINTF("DISPATCHING: Bundle forwarded\n");
}
/** @} */
