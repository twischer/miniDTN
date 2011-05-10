/**
 * \addtogroup agent
 * @{
 */
 
 /**
 * \defgroup admin_record Erstellen eins Administrative Records (Custody Signal)
 *
 *  Bisher existiert nur das Custody Signal, erweiterung möglich
 *
 * @{
 */
 
 /**
 * \file
 *         
 */
#ifndef ADMINISTRATIVE_RECORD_H
#define ADMINISTRATIVE_RECORD_H

#include "API/DTN-block-types.h"
#include "net/dtn/custody-signal.h"
#include "net/dtn/status-report.h"

/*  Defines */

/**
*   \brief Definition für das Custody Signal als Administrative Record Typ
*
*/
#define CUSTODY_SIGNAL					(0x20)

/* nicht implementiert */
#define BUNDLE_STATUS_REPORT				(0x10)
#define IS_FOR_FRAGMENT					(0x01)

/**
*   \brief PCF im Primary Block für ein Administrative Record Bündel
*/
#define ADMIN_RECORD_PRIMARY_PCF			(0x12)


/**
*  \brief Struktur für ein Administrative Record
*   
*/
typedef struct {
	uint8_t record_status; 		/** 8 bit, 4 bit Record Type Code, 4 bit Record Flags */
	union {
		custody_signal_t custody_signal; /** Custody Signal Struktur */
		status_report_t status_report; 
	};
} administrative_record_block_t;

#endif
/** @} */
/** @} */
