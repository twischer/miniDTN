/**
* \file
*
*/
#ifndef STATUS_REPORT_H
#define STATUS_REPORT_H

#include <stdint.h>
#include "net/dtn/bundle.h"
#include "mmem.h"


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
	uint8_t status;
	uint8_t reason_code;
	struct mmem mem;
} status_report_t;

struct status_report_driver {
	char *name;
	uint8_t (* send)(struct bundle_t *bundle,uint8_t status, uint8_t reason);
};

extern const struct status_report_driver STATUS_REPORT;

#endif
