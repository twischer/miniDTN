/**
 * \addtogroup agent
 * @{
 */

 /**
 * \defgroup api Bundle Protocoll API
 *
 * @{
 */

/**
 * \file 
 * this file defines the API events
 *        
 */
#ifndef API_EVENTS_H
#define API_EVENTS_H

#include "contiki.h"
/**
*   \brief Event to register a Service
 */
process_event_t dtn_application_registration_event;
/**
*   \brief Event to delete a registration of a Service
*/
process_event_t dtn_application_remove_event;
/**
*   \brief Event to chage a services state
*/
process_event_t dtn_application_status_event;
/**
*   \brief Event to send a created bundle
*/
process_event_t dtn_send_bundle_event;
/**
*   \brief Event submit received bundles to a registered service
*/
process_event_t submit_data_to_application_event;
/**
 * \brief Event tell the agent that the service has finished processing a particular bundle
 */
process_event_t dtn_processing_finished;
/**
 * \brief Event tell the service, that the outgoing bundle has reached save grounds
 */
process_event_t dtn_bundle_stored;

#endif
/** @} */
/** @} */

