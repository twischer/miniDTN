/**
 * \addtogroup agent
 *
 * @{
 */

/**
 * \defgroup admininistrative_records Administrative Records
 *
 * @{
 */

/**
 * \file
 * \brief Defines admin records
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef ADMINISTRATIVE_RECORD_H
#define ADMINISTRATIVE_RECORD_H

#include "custody_signal.h"
#include "statusreport.h"

/**
 * Administrative Record Type Codes
 */
#define TYPE_CODE_BUNDLE_STATUS_REPORT		(0x01 << 4)
#define TYPE_CODE_CUSTODY_SIGNAL			(0x02 << 4)

/**
 * Administrative Record Flags
 */
#define ADMIN_FLAGS_IS_FOR_FRAGMENT			(0x01)

#define ADMIN_RECORD_PRIMARY_PCF			(0x12)

typedef struct {
	uint8_t record_status; 		/** 8 bit, 4 bit Record Type Code, 4 bit Record Flags */
	union {
		custody_signal_t custody_signal; /** Custody Signal Struct */
		status_report_t status_report; 
	};
} administrative_record_block_t;

#endif
