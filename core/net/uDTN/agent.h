 /**
 * \defgroup agent Bundle Protocol
 *
 * @{
 */
 
 /**
 * \defgroup bprocess Bundle Protocol Process
 *
 * @{
 */
/** 
* \file
*
*/
 

#ifndef AGENT_H
#define AGENT_H

#include "contiki.h"

extern uint32_t dtn_node_id;
extern uint32_t dtn_seq_nr;
uint16_t g_bundle_num;
PROCESS_NAME(agent_process);

/**
*  \name Events internal communication 
* @{
*/
/* Event thrown to the bundle agent by network layer */
process_event_t dtn_receive_bundle_event;

/* Event thrown to the bundle agent by storage */
process_event_t dtn_bundle_in_storage_event;

/* Event to transmit an administrative record */
process_event_t dtn_send_admin_record_event;

/* Event to transmit a bundle to a specific node */
process_event_t dtn_send_bundle_to_node_event;
/** @} */
/**
*   \brief Bundle Protocols initialisation
*
*    called by contikis main function
*&agent_process
*/
void agent_init(void);

void agent_del_bundle(void);

#endif

/** @} */
/** @} */
