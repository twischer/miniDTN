 /**
 * \defgroup agent Der Bundle Protocol Agent
 *
 * @{
 */
 
 /**
 * \defgroup process Der Bundle Protocol Prozess
 *
 * @{
 */
 
 /**
 * \file
 *         Headerfile des Prozesses
 *
 */

#ifndef AGENT_H
#define AGENT_H

#include "contiki.h"

extern uint32_t dtn_node_id, dtn_seq_nr;
uint16_t g_bundle_num;
/**
*   \brief Makro für den Namen des Prozesses
*/
PROCESS_NAME(agent_process);

/**
*  \name Events für die im Agent interne Kommunikation 
* @{
*/
/* Event thrown to the bundle agent by network layer */
process_event_t dtn_receive_bundle_event;
process_event_t dtn_bundle_in_storage_event;

/* Event to transmit an administrative record */
process_event_t dtn_send_admin_record_event;
process_event_t dtn_send_bundle_to_node_event;
/** @} */
void agent_init(void);

#endif

/** @} */
/** @} */
