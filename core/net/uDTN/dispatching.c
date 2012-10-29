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
//#define ENABLE_LOGGING 1
#include "logging.h"
#include "storage.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


void dispatch_bundle(struct mmem *bundlemem) {
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	uint32_t bundle_number = 0;

	if ((bundle->flags & BUNDLE_FLAG_ADM_REC) && (bundle->dst_node == dtn_node_id)) {
		// The bundle is an ADMIN RECORD for our node, process it directly here without going into storage

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

		// Decrease the reference counter - this should deallocate the bundle
		bundle_dec(bundlemem);

		// Exit function, nothing else to do here
		return;
	}

	// Now pass on the bundle to storage
	if (bundle->flags & 0x08){
		// bundle is custody
		PRINTF("FORWARDING: Handing over to custody\n");

		CUSTODY.decide(bundlemem, &bundle_number);
		return;
	}

	// regular bundle, no custody
	PRINTF("FORWARDING: Handing over to storage\n");
	BUNDLE_STORAGE.save_bundle(bundlemem, &bundle_number);
}
/** @} */
