/**
 * \addtogroup admininistrative_records
 * @{
 */

/**
 * \file
 * \brief Definition of Custody Signals
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef CUSTODY_SIGNAL_H
#define CUSTODY_SIGNAL_H

#include <stdint.h>

#include "bundle.h"

#define CUSTODY_TRANSFER_SUCCEEDED		(0x80)
#define CUSTODY_TRANSFER_FAILED			(0x00)
#define NO_ADDITIONAL_INFORMATION			(0x00)
#define REDUNDANT_RECEPTION				(0x03)
#define DEPLETED_STORAGE					(0x04)
#define DEST_EID_UNINTELLIGIBLE				(0x05)
#define NO_KNOWN_ROUTE_TO_DEST			(0x06)
#define NO_TIMELY_CONTACT_WITH_NEXT_NODE	(0x07)
#define BLOCK_UNINTELLIGIBLE				(0x08)
typedef struct {
	
	uint8_t status; 
	uint32_t fragement_offset;
	uint32_t fragment_length;
	uint32_t custody_signal_time;
	uint32_t bundle_creation_timestamp;
	uint32_t bundle_creation_timestamp_seq;
	uint32_t src_node;
	uint32_t src_app;
} custody_signal_t;

#endif
