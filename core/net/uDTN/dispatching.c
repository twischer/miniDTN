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
#include "hash.h"
#include "redundancy.h"

#include "dispatching.h"

/**
 * \brief This function checks, whether an incoming admin record is a bundle delivery report. If so, the corresponding bundle is deleted from storage
 * \param bundlemem Pointer to the MMEM struct containing the bundle
 * \return 1 on success, < 0 otherwise
 */
int dispatching_check_report(struct mmem * bundlemem) {
	struct bundle_t * bundle = NULL;
	struct bundle_block_t * payload_block = NULL;
	status_report_t report;
	int ret = 0;
	uint32_t bundle_number = 0;

	/* Get the bundle */
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	/* Return on invalid pointer */
	if( bundle == NULL ) {
		return -1;
	}

	/* Get the payload block pointer */
	payload_block = bundle_get_payload_block(bundlemem);

	/* Check if the bundle has a payload block */
	if( payload_block == NULL ) {
		return -1;
	}

	/* Check if the block contains a status report */
	if( !(payload_block->payload[0] & TYPE_CODE_BUNDLE_STATUS_REPORT) ) {
		return -1;
	}

	/* Decode the status report */
	ret = STATUSREPORT.decode(&report, payload_block->payload, payload_block->block_size);

	/* Do not continue on error */
	if( ret < 0 ) {
		return -1;
	}

	/* Abort of no delivery report */
	if( !(report.status_flags & NODE_DELIVERED_BUNDLE) ) {
		return -1;
	}

	/* Calculate bundle number */
	bundle_number = HASH.hash_convenience(report.bundle_sequence_number, report.bundle_creation_timestamp, report.source_eid_node, report.source_eid_service, report.fragment_offset, report.fragment_length);

	LOG(LOGD_DTN, LOG_AGENT, LOGL_INF, "Received delivery report for bundle %lu from ipn:%lu, deleting", bundle_number, bundle->src_node);

	/* And delete the bundle without sending additional reports */
	BUNDLE_STORAGE.del_bundle(bundle_number, REASON_DELIVERED);

	return 1;
}

int dispatching_dispatch_bundle(struct mmem *bundlemem) {
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	uint32_t * bundle_number_ptr;
	uint32_t bundle_number = 0;
	int n;
	uint8_t received_report = 0;
	uint32_t payload_length = 0;

	/* If we receive a delivery report for a bundle, delete the corresponding bundle from storage */
	if( bundle->flags & BUNDLE_FLAG_ADM_REC ) {
		dispatching_check_report(bundlemem);
		bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	}

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
	if (bundle->flags & BUNDLE_FLAG_CUST_REQ){
		// bundle is custody
		LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Handing over to custody");

		CUSTODY.decide(bundlemem, bundle_number_ptr);
		return 1;
	}

	// To uniquely identify fragments, we need the length of the payload block
	if( bundle->flags & BUNDLE_FLAG_FRAGMENT ) {
		struct bundle_block_t * payload_block = bundle_get_payload_block(bundlemem);
		payload_length = payload_block->block_size;
	}

	// Calculate the bundle number
	bundle_number = HASH.hash_convenience(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->src_srv, bundle->frag_offs, payload_length);
	bundle->bundle_num = bundle_number;

	// Check if the bundle has been delivered before
	if( REDUNDANCE.check(bundle_number) ) {
		bundle_decrement(bundlemem);

		// If the bundle is redundant we still have to report success to make the CL send an ACK
		return 1;
	}

	// Does the sender want a "received" status report?
	if( bundle->flags & BUNDLE_FLAG_REP_RECV ) {
		received_report = 1;
	}

	// regular bundle, no custody
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Handing over to storage");
	n = BUNDLE_STORAGE.save_bundle(bundlemem, &bundle_number_ptr);
	bundlemem = NULL;

	// Send out a "received" status report if requested
	if( received_report ) {
		// Read back from storage
		bundlemem = BUNDLE_STORAGE.read_bundle(bundle_number);

		if( bundlemem != NULL) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Sending out delivery report for bundle %lu", bundle_number);

			// Send out report
			STATUSREPORT.send(bundlemem, NODE_RECEIVED_BUNDLE, NO_ADDITIONAL_INFORMATION);

			// Free memory
			bundle_decrement(bundlemem);
		}
	}

	if( n ) {
		// Put the bundle into the list of already seen bundles
		REDUNDANCE.set(bundle_number);

		// Now we have to send an event to our daemon
		process_post(&agent_process, dtn_bundle_in_storage_event, bundle_number_ptr);

		return 1;
	}

	return 0;
}
/** @} */
