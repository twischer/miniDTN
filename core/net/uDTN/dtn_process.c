/**
 * \addtogroup agent
 * @{
 */

/**
 * \file
 * \brief miniDTN process managment
 * \author Timo Wischer <t.wischer@tu-bs.de>
 */


#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "api.h"

bool dtn_process_create(TaskFunction_t pvTaskCode, const char * const pcName)
{
	// TODO add parameter and fail if parameter false and function already used for an process


	TaskHandle_t createdTask;
	if ( !xTaskCreate(pvTaskCode, pcName, configMINIMAL_STACK_SIZE, NULL, 1, &createdTask) ) {
		return false;
	}

	const QueueHandle_t event_queue = xQueueCreate( 10, sizeof(event_container_t) );
	if (event_queue == NULL) {
		return false;
	}

	/*
	 * The created event queue will be used as input queue for
	 * the created task.
	 * So this queue belong to this task.
	 * Save the queue as the application tag of this task.
	 */
	vTaskSetApplicationTaskTag(createdTask, event_queue);

	return true;
}


QueueHandle_t dtn_process_get_event_queue()
{
	/* get the application tag of the calling task */
	return (QueueHandle_t)xTaskGetApplicationTaskTag(NULL);
}


void dtn_process_send_event(const QueueHandle_t event_queue, const event_t event, const void* const data)
{
	if (event_queue == NULL) {
//		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Event queue not defined!");
		return;
	}

	const event_container_t event_container = {
		.event = event,
		.data = data
	};

	if ( !xQueueSend(event_queue, &event_container, 0) ) {
//		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Could not send the event!");
	}
}
