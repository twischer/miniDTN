/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup api Bundle Protocol API
 *
 * @{
 */

/**
 * \file
 * \brief uDTN API
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef API_H
#define API_H

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
/**
 * \brief Event tell the service, that the outgoing bundle could not be saved
 */
process_event_t dtn_bundle_store_failed;

#ifndef APP_ACTIVE
#define APP_ACTIVE	1
#endif

#ifndef APP_PASSIVE
#define APP_PASSIVE	0
#endif

/**
 * \brief Struct for service registration
 */
struct registration_api {
	uint32_t app_id;
	uint8_t status:1;
	uint32_t node_id;
	struct process *application_process;
};

#endif /* API_H */
/** @} */
