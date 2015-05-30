/**
 * \addtogroup agent
 * @{
 */

/**
 * \file
 * \brief miniDTN process managment
 * \author Timo Wischer <t.wischer@tu-bs.de>
 */


#include "dtn_process.h"


bool dtn_process_create(const TaskFunction_t pvTaskCode, const char* const pcName)
{
	return dtn_process_create_other_stack(pvTaskCode, pcName, configMINIMAL_STACK_SIZE);
}


bool dtn_process_create_other_stack(const TaskFunction_t pvTaskCode, const char* const pcName, const uint16_t usStackDepth)
{
	// TODO add parameter and fail if parameter false and function already used for an process


	TaskHandle_t createdTask;
	if ( !xTaskCreate(pvTaskCode, pcName, usStackDepth, NULL, 1, &createdTask) ) {
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


bool dtn_process_wait_any_event(const TickType_t xTicksToWait, event_container_t* const event_container)
{
	return xQueueReceive(dtn_process_get_event_queue(), event_container, xTicksToWait);
}

bool dtn_process_wait_event(const event_t event, const TickType_t xTicksToWait, event_container_t* const event_container)
{
	const TickType_t wait_start = xTaskGetTickCount();
	do {
		const TickType_t ticks_left = xTicksToWait - (xTaskGetTickCount() - wait_start);
		if ( !dtn_process_wait_any_event(ticks_left, event_container) ) {
			/* timeout has expired, so cancel waiting for the event */
			return false;
		}
	} while (event_container->event != event);

	return true;
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
