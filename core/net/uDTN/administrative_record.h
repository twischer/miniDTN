#ifndef ADMINISTRATIVE_RECORD_H
#define ADMINISTRATIVE_RECORD_H

#include "custody_signal.h"
#include "statusreport.h"

#define CUSTODY_SIGNAL					(0x20)

#define BUNDLE_STATUS_REPORT				(0x10)
#define IS_FOR_FRAGMENT					(0x01)

#define ADMIN_RECORD_PRIMARY_PCF			(0x12)


typedef struct {
	uint8_t record_status; 		/** 8 bit, 4 bit Record Type Code, 4 bit Record Flags */
	union {
		custody_signal_t custody_signal; /** Custody Signal Struktur */
		status_report_t status_report; 
	};
} administrative_record_block_t;

#endif
