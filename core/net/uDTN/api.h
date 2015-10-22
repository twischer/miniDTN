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

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "net/linkaddr.h"

typedef enum {
	/**
	*   \brief Event to register a Service
	 */
	dtn_application_registration_event,
	/**
	*   \brief Event to delete a registration of a Service
	*/
	dtn_application_remove_event,
	/**
	*   \brief Event to chage a services state
	*/
	dtn_application_status_event,
	/**
	*   \brief Event to send a created bundle
	*/
	dtn_send_bundle_event,
	/**
	*   \brief Event submit received bundles to a registered service
	*/
	submit_data_to_application_event,
	/**
	 * \brief Event tell the agent that the service has finished processing a particular bundle
	 */
	dtn_processing_finished,
	/**
	 * \brief Event tell the service, that the outgoing bundle has reached save grounds
	 */
	dtn_bundle_stored,
	/**
	 * \brief Event tell the service, that the outgoing bundle could not be saved
	 */
	dtn_bundle_store_failed,

	/**
	*  \name Events internal communication
	* @{
	*/
	/* Event thrown to the bundle agent by network layer */
	dtn_receive_bundle_event,

	/* Event thrown to the bundle agent by storage */
	dtn_bundle_in_storage_event,

	/* Event to transmit an administrative record */
	dtn_send_admin_record_event,

	/* Event to transmit a bundle to a specific node */
	dtn_send_bundle_to_node_event,

	dtn_beacon_event,

	dtn_disco_start_event,
	dtn_disco_stop_event,
	/** @} */

} event_t;

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
	QueueHandle_t event_queue;
};

typedef struct {
	event_t event;
	union {
		const void* const data;
		/* used by dtn_send_bundle_event, dtn_processing_finished */
		struct mmem* bundlemem;
		/* used by dtn_application_registration_event, dtn_application_status_event, dtn_application_remove_event */
		struct registration_api* registration;
		/* used by dtn_bundle_in_storage_event */
		uint32_t bundle_number;
		/* used by dtn_beacon_event */
		linkaddr_t* linkaddr;
	};
} event_container_t;


#endif /* API_H */
/** @} */
