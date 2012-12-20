/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "lib/list.h"
#include "logging.h"

#include "agent.h"
#include "bundle.h"
#include "registration.h"
#include "sdnv.h"
#include "administrative_record.h"
#include "custody.h"
#include "delivery.h"
#include "statusreport.h"
#include "storage.h"

#include "dispatching.h"

int dispatching_dispatch_bundle(struct mmem *bundlemem) {
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	uint32_t * bundle_number;
	int n;
	uint8_t received_report = 0;

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
		bundle_decrement(bundlemem);

		// Exit function, nothing else to do here
		return 1;
	}

	// Now pass on the bundle to storage
	if (bundle->flags & 0x08){
		// bundle is custody
		LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Handing over to custody");

		CUSTODY.decide(bundlemem, bundle_number);
		return 1;
	}

	// Does the sender want a "received" status report?
	if( bundle->flags & BUNDLE_FLAG_REP_RECV ) {
		received_report = 1;
	}

	// regular bundle, no custody
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Handing over to storage");
	n = BUNDLE_STORAGE.save_bundle(bundlemem, &bundle_number);
	bundlemem = NULL;

	// Send out a "received" status report if requested
	if( received_report ) {
		// Read back from storage
		bundlemem = BUNDLE_STORAGE.read_bundle(*bundle_number);

		if( bundlemem != NULL) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Sending out delivery report for bundle %lu", *bundle_number);

			// Send out report
			STATUSREPORT.send(bundlemem, NODE_RECEIVED_BUNDLE, NO_ADDITIONAL_INFORMATION);

			// Free memory
			bundle_decrement(bundlemem);
		}
	}

	// Now we have to send an event to our daemon
	if( n ) {
		process_post(&agent_process, dtn_bundle_in_storage_event, bundle_number);
		return 1;
	}

	return 0;
}
/** @} */
