/**
 * \addtogroup agent
 * @{
 */

/**
 * \file
 * \brief miniDTN process managment
 * \author Timo Wischer <t.wischer@tu-bs.de>
 */

#ifndef DTN_PROCESS_H
#define DTN_PROCESS_H

#define DTN_QUEUE_LENGTH	10

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "api.h"


bool dtn_process_create(const TaskFunction_t pvTaskCode, const char* const pcName);
bool dtn_process_create_other_stack(const TaskFunction_t pvTaskCode, const char* const pcName, const uint16_t usStackDepth);
bool dtn_process_create_with_queue(const TaskFunction_t pvTaskCode, const char* const pcName, const uint16_t usStackDepth,
								   const UBaseType_t uxPriority, QueueHandle_t* const event_queue);


/**
 * @brief dtn_process_get_event_queue
 * @return the input event queue of the current dtn process
 * Be carful. The dtn proccess have to be created by dtn_process_create.
 * Otherwise the queue will not returned.
 */
QueueHandle_t dtn_process_get_event_queue();

bool dtn_process_wait_any_event(const TickType_t xTicksToWait, event_container_t* const event_container);
bool dtn_process_wait_event(const event_t event, const TickType_t xTicksToWait, event_container_t* const event_container);


void dtn_process_send_event(const QueueHandle_t event_queue, const event_t event, const void* const data);


#endif /* DTN_PROCESS_H */
