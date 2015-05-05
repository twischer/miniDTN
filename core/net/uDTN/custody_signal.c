/**
 * \addtogroup admininistrative_records
 * @{
 */

/**
 * \file
 * \brief Implementation of Custody Signals
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#include <stdlib.h>
#include <string.h>

#include "sys/clock.h"

#include "sdnv.h"
#include "custody.h"

#include "custody_signal.h"

void custody_signal_received(custody_signal_t *custody_signal) {
#if 0
	if((custody_signal->status & CUSTODY_TRANSFER_SUCCEEDED) != 0)
	{
		custody_remove_bundle(custody_signal->src_node, custody_signal->src_app, custody_signal->bundle_creation_timestamp, custody_signal->bundle_creation_timestamp_seq);
	}
	else if((custody_signal->status & CUSTODY_TRANSFER_SUCCEEDED) == 0) {
		
		if((custody_signal->status & REDUNDANT_RECEPTION) != 0) {
			
			custody_remove_bundle(custody_signal->src_node, custody_signal->src_app, custody_signal->bundle_creation_timestamp, custody_signal->bundle_creation_timestamp_seq);
		}
	}
#endif
}	

void custody_signal_set_signal_status (uint8_t reason, uint8_t transfer_result, custody_signal_t *return_signal) {
#if 0
	return_signal->status = transfer_result | reason;
#endif
}

void custody_signal_set_signal_time(custody_signal_t *return_signal) {
#if 0
	return_signal->custody_signal_time = clock_time();
#endif
}

void custody_signal_copy_timestamp_to_signal (struct bundle_t *bundle , custody_signal_t *return_signal) {
#if 0
	sdnv_decode(bundle->mem->ptr + bundle->offset_tab[TIME_STAMP][OFFSET], bundle->offset_tab[TIME_STAMP][STATE], return_signal->bundle_creation_timestamp);
	sdnv_decode(bundle->mem->ptr + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], bundel->offset_tab[TIME_STAMP_SEQ_NR][STATE], return_signal->bundle_creation_timestamp_seq);
#endif
}

void custody_signal_copy_eid_to_signal(struct bundle_t *bundle, custody_signal_t *return_signal) {
#if 0
	sdnv_decode(bundle->mem->ptr + bundle->offset_tab[SRC_NODE][OFFSET], bundle->offset_tab[SRC_NODE][STATE], return_signal->src_node);

	sdnv_decode(bundle->mem->ptr + bundle->offset_tab[SRC_APP][OFFSET], bundle->offset_tab[SRC_APP][STATE], return_signal->src_app);
#endif
}
/** @} */
