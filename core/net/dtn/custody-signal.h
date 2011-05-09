 /**
 * \addtogroup agent
 * @{
 */
 
  /**
 * \addtogroup admin_record
 * @{
 */

/**
*   \file
*   
*/
#ifndef CUSTODY_SIGNAL_H
#define CUSTODY_SIGNAL_H

#include <stdint.h>

#include "bundle.h"
#include "dtn_config.h"

/*  Defines */

/**
*
*   \name Definitionen des Custody Transfer Ergebnisses
*/
#define CUSTODY_TRANSFER_SUCCEEDED		(0x80)
#define CUSTODY_TRANSFER_FAILED			(0x00)
/**    @} */

/**
*
*   \name Definition der Gründe für ein Custody Signal
*   @{
*/
#define NO_ADDITIONAL_INFORMATION			(0x00)
#define REDUNDANT_RECEPTION				(0x03)
#define DEPLETED_STORAGE					(0x04)
#define DEST_EID_UNINTELLIGIBLE				(0x05)
#define NO_KNOWN_ROUTE_TO_DEST			(0x06)
#define NO_TIMELY_CONTACT_WITH_NEXT_NODE	(0x07)
#define BLOCK_UNINTELLIGIBLE				(0x08)
/**    @} */

/**
*   \brief Strukur die den Aufbau eines Custody Signals wiederspiegelt
*/
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


/**
*   \brief Verarbeitet empfangenes Custody Signal
*
*   ergreift entsprechend des Status vordefinierte Aktionen
*
*   \param custody_signal custody_signal_t Struktur die die Infomationen enthält
*/
void custody_signal_received(custody_signal_t *custody_signal);

/**
*   \brief Setzt den Signal Status in eines Custody Signals
*
*   \param reason Grund des Custody Signals
*   \param transfer_result Ergebnis der Custody Prozedur
*   \param return_signal Zeiger auf custody signal stuktur, die gesendet werden soll
*/
void set_signal_status (uint8_t reason, uint8_t transfer_result, custody_signal_t *return_signal);

/**
*   \brief Setzt die Zeit in einer Custody Signal Struktur
*
*   \param return_signal Zeiger auf die Struktur, in der die Zeit gesetzt werden soll
*/
void set_signal_time(custody_signal_t *return_signal);

/**
*   \brief Kopiert Zeitstempel und Seq-Nr des Custody Signal verusachenden Bündel in das Custody Signal
*
*   \param block Zeiger auf Primary Block, des verursachenden Bündels
*   \param return_signal Zeiger auf das Custody Signal
*/
void copy_timestamp_to_signal (struct bundle_t *bundle, custody_signal_t *return_signal);

/**
*   \brief Kopiert Source EID des verursachenden Bündels ins Custody Signal
*
*   \param block Zeiger auf Primary Block, des verursachenden Bündels
*   \param return_signal Zeiger auf das Custody Signal
*/
void copy_eid_to_signal(bundle_t *bundle, custody_signal_t *return_signal);


#endif
/** @} */
/** @} */
