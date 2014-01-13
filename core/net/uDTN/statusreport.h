/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup bundle_status Bundle Status reports
 * @{
 */

/**
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef STATUS_REPORT_H
#define STATUS_REPORT_H

#include <stdint.h>

#include "contiki.h"
#include "mmem.h"

#include "bundle.h"

/**
 * Which status-report driver are we going to use?
 */
#ifdef CONF_STATUSREPORT
#define STATUSREPORT CONF_STATUSREPORT
#else
#define STATUSREPORT statusreport_basic
#endif

/*========================================== STATUS FLAGS ==========================================*/

/**
 * \brief The status flags for the status report
 * @{
 */
#define NODE_RECEIVED_BUNDLE				(0x01)
#define NODE_ACCEPTED_CUSTODY				(0x02)
#define NODE_FORWARDED_BUNDLE				(0x04)
#define NODE_DELIVERED_BUNDLE				(0x08)
#define NODE_DELETED_BUNDLE					(0x10)
/** @} */

/*================================ STATUS REPORT REASON CODES =================================*/

/**
 * \brief The reasons for the status report
 * @{
 */
#define NO_ADDITIONAL_INFORMATION			(0x00)
#define LIFETIME_EXPIRED					(0x01)
#define FORWARDED_OVER_UNIDIRECTIONAL_LINK	(0x02)
#define TRANSMISSION_CANCELED				(0x03)
#define DEPLETED_STORAGE					(0x04)
#define DEST_EID_UNINTELLIGIBLE				(0x05)
#define NO_KNOWN_ROUTE_TO_DEST				(0x06)
#define NO_TIMELY_CONTACT_WITH_NEXT_NODE	(0x07)
#define BLOCK_UNINTELLIGIBLE				(0x08)
/** @} */

/**
 * \brief The structure of a status report as part of an administrative record
 */
typedef struct {
	uint8_t status_flags;
	uint8_t reason_code;
	uint8_t fragment;

	uint32_t fragment_offset;
	uint32_t fragment_length;

	uint32_t dtn_time_seconds;
	uint32_t dtn_time_nanoseconds;

	uint32_t bundle_creation_timestamp;
	uint32_t bundle_sequence_number;

	uint32_t source_eid_node;
	uint32_t source_eid_service;
} status_report_t;

/**
 * interface for status report modules
 */
struct status_report_driver {
	char *name;

	/** sends a status report to the "report to"-node */
	uint8_t (* send)(struct mmem * bundlemem, uint8_t status, uint8_t reason);

	/* Encode a status report struct into a buffer
	 * Returns < 0 on error
	 * Returns >= 0 on scucess
	 */
	int (* encode)(status_report_t * report, uint8_t * buffer, uint8_t length);

	/* Decode a status report buffer into the struct
	 * Returns < 0 on error
	 * Returns >= 0 on scucess
	 */
	int (* decode)(status_report_t * report, uint8_t * buffer, uint8_t length);
};

extern const struct status_report_driver STATUSREPORT;

#endif
/** @} */
/** @} */
