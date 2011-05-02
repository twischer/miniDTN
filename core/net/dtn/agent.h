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

extern uint32_t dtn_node_id;
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

/* Event to transmit an administrative record */
process_event_t dtn_send_admin_record_event;
/** @} */
void agent_init(void);

#endif

/** @} */
/** @} */
