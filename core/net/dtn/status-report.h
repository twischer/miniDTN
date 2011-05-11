/**
* \file
*
*/
#ifndef STATUS_REPORT_H
#define STATUS_REPORT_H

#include <stdint.h>
#include "net/dtn/bundle.h"


/*========================================== STATUS FLAGS ==========================================*/

/**
*   \brief The status flags for the status report
*    @{
*/
#define NODE_RECEIVED_BUNDLE		(0x01)
#define NODE_ACCEPTED_CUSTODY	(0x02)
#define NODE_FORWARDED_BUNDLE	(0x04)
#define NODE_DELIVERED_BUNDLE		(0x08)
#define NODE_DELETED_BUNDLE		(0x10)
/**   @} */

/*================================ STATUS REPORT REASON CODES =================================*/

/**
*   \brief The reasons for the status report
*    @{
*/
#define NO_ADDITIONAL_INFORMATION			(0x00)
#define LIFETIME_EXPIRED					(0x01)
#define FORWARDED_OVER_UNIDIRECTIONAL_LINK	(0x02)
#define TRANSMISSION_CANCELED				(0x03)
#define DEPLETED_STORAGE					(0x04)
#define DEST_EID_UNINTELLIGIBLE				(0x05)
#define NO_KNOWN_ROUTE_TO_DEST			(0x06)
#define NO_TIMELY_CONTACT_WITH_NEXT_NODE	(0x07)
#define BLOCK_UNINTELLIGIBLE				(0x08)
/**   @} */

/**
*   \brief The structure of a status report as part of an administrative record
*/
typedef struct {
	uint8_t status_flag;
	uint8_t reason_code;
	uint16_t fragement_offset;
	uint16_t fragment_length;
	union {
		uint32_t bundle_receipt_time;
		uint32_t bundle_custody_time;
		uint32_t bundle_forwarding_time;
		uint32_t bundle_delivery_time;
		uint32_t bundle_deletion_time;
	};
	uint32_t bundle_creation_timestamp;
	uint8_t bundle_creation_timestamp_seq;
	uint8_t source_eid_length;
	char *source_eid;
} status_report_t;

#endif
