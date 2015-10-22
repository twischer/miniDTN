/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \brief Bundle Slot memory management
 *
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 */
#include "bundleslot.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "lib/logging.h"

#include "bundle.h"
#include "storage.h"
#include "agent.h"

/* Defines how many bundles can be used (in storage, used) on this node at once */
#ifdef CONF_BUNDLE_NUM
#define BUNDLE_NUM CONF_BUNDLE_NUM
#else
#define BUNDLE_NUM (BUNDLE_STORAGE_SIZE + 10)
#endif

static SemaphoreHandle_t mutex = NULL;
static struct bundle_slot_t bundleslots[BUNDLE_NUM];
static uint8_t slots_in_use = 0;


int bundleslot_init()
{
	/* Only execute the initalisation before the scheduler was started.
	 * So there exists only one thread and
	 * no locking is needed for the initialisation
	 */
	configASSERT(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED);

	/* cancle, if initialisation was already done */
	if(mutex != NULL) {
		return -1;
	}
	mutex = xSemaphoreCreateRecursiveMutex();
	if(mutex == NULL) {
		return -2;
	}

	memset(bundleslots, 0, sizeof(bundleslots));

	return 1;
}

/* Returns a pointer to a newly allocated bundle */
struct bundle_slot_t *bundleslot_get_free()
{
	/* enter the critical section */
	if ( !xSemaphoreTakeRecursive(mutex, portMAX_DELAY) ) {
		return NULL;
	}

	for (uint16_t i=0; i<BUNDLE_NUM; i++) {
		if (bundleslots[i].ref == 0) {
			memset(&bundleslots[i], 0, sizeof(struct bundle_slot_t));

			bundleslots[i].ref++;
			bundleslots[i].type = 0;
			slots_in_use ++;

			xSemaphoreGiveRecursive(mutex);
			return &bundleslots[i];
		}
	}

	xSemaphoreGiveRecursive(mutex);
	return NULL;
}

/* Frees the bundle */
int bundleslot_free(struct bundle_slot_t *bs)
{
	/* enter the critical section */
	if ( !xSemaphoreTakeRecursive(mutex, portMAX_DELAY) ) {
		return -1;
	}

	if( bs->bundle.ptr == NULL ) {
		LOG(LOGD_DTN, LOG_SLOTS, LOGL_ERR, "DUPLICATE FREE");

		xSemaphoreGiveRecursive(mutex);
		return -2;
	}

	bs->ref = 0;
	slots_in_use --;

	LOG(LOGD_DTN, LOG_SLOTS, LOGL_DBG, "bundleslot_free(%p) %u", bs, slots_in_use);

	// Zeroify all freed memory
	memset(bs->bundle.ptr, 0, sizeof(struct bundle_t));

	mmem_free(&bs->bundle);

	// And kill the pointer!
	bs->bundle.ptr = NULL;

	xSemaphoreGiveRecursive(mutex);
	return 0;
}

/* Increment usage count */
int bundleslot_increment(struct bundle_slot_t *bs)
{
	/* enter the critical section */
	if ( !xSemaphoreTakeRecursive(mutex, portMAX_DELAY) ) {
		return -2;
	}

	LOG(LOGD_DTN, LOG_SLOTS, LOGL_DBG, "bundleslot_inc(%p) to %u", bs, bs->ref+1);

	if (!bs->ref) {
		xSemaphoreGiveRecursive(mutex);
		return -1;
	}

	bs->ref++;

	xSemaphoreGiveRecursive(mutex);
	// TODO save in temp variable
	return bs->ref;
}

/* Decrement usage count, free if necessary */
int bundleslot_decrement(struct bundle_slot_t *bs)
{
	/* enter the critical section */
	if ( !xSemaphoreTakeRecursive(mutex, portMAX_DELAY) ) {
		return -2;
	}

	LOG(LOGD_DTN, LOG_SLOTS, LOGL_DBG, "bundleslot_dec(%p) to %u", bs, bs->ref-1);

	if (!bs->ref) {
		xSemaphoreGiveRecursive(mutex);
		return -1;
	}

	bs->ref--;
	if (!bs->ref)
		bundleslot_free(bs);

	xSemaphoreGiveRecursive(mutex);
	// TODO save in temp variable
	return bs->ref;
}
