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

#include <stdbool.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "api.h"
#include "bundle.h"

#include "contiki.h"

extern uint32_t dtn_node_id;
extern uint32_t dtn_seq_nr;


/**
*   \brief Bundle Protocols initialisation
*
*    called by contikis main function
*/
bool agent_init(void);
void agent_delete_bundle(uint32_t bundle_number);
uint32_t agent_get_sequence_number();
void agent_set_sequence_number(uint32_t);
void agent_set_bundle_source(struct bundle_t* const bundle);
void agent_send_event(const event_container_t* const event);

/**
 * \brief uDTN log domains
 */
#define LOG_NET    				0
#define LOG_BUNDLE 				1
#define LOG_ROUTE  				2
#define LOG_STORE  				3
#define LOG_SDNV   				4
#define LOG_SLOTS  				5
#define LOG_AGENT  				6
#define LOG_CL	   				7
#define LOG_DISCOVERY			8
#define LOG_DISCOVERY_SCHEDULER 9
#endif

/** @} */
/** @} */
/** @} */
