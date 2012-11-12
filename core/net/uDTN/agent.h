/**
 * \addtogroup net
 * @{
 */

/**
 * \defgroup agent uDTN: Bundle Protocol stack
 *
 * @{
 */

/**
 * \defgroup bprocess Bundle Protocol Agent
 *
 * @{
 */

/** 
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef AGENT_H
#define AGENT_H

#include "contiki.h"

extern uint32_t dtn_node_id;
extern uint32_t dtn_seq_nr;
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
*/
void agent_init(void);
void agent_delete_bundle(uint32_t bundle_number);

/**
 * \brief uDTN log domains
 */
#define LOG_NET    		0
#define LOG_BUNDLE 		1
#define LOG_ROUTE  		2
#define LOG_STORE  		3
#define LOG_SDNV   		4
#define LOG_SLOTS  		5
#define LOG_AGENT  		6
#define LOG_CL	   		7
#define LOG_DISCOVERY	8
#endif

/** @} */
/** @} */
/** @} */
