/**
 * \addtogroup discovery
 * @{
 */

/**
 * \defgroup discovery_scheduler_periodic Periodic (and always on) discovery scheduler module
 *
 * @{
 */

/**
 * \file
 * \brief Pattern discovery scheduler module
 *
 * \author Wolf-Bastian Pöttner <poettner@ibr.cs.tu-bs.de>
 */
#include "discovery_scheduler.h"
#include "discovery.h"

#include "FreeRTOS.h"
#include "timers.h"

#include "sys/ctimer.h"

static TimerHandle_t dst;

void discovery_scheduler_periodic_func(const TimerHandle_t timer);

bool discovery_scheduler_periodic_init() {
//	ctimer_set(&dst, DISCOVERY_CYCLE * CLOCK_SECOND, discovery_scheduler_periodic_func, NULL);
	dst = xTimerCreate("discovery scheduler timer", pdMS_TO_TICKS(DISCOVERY_CYCLE * 1000), pdFALSE, NULL, discovery_scheduler_periodic_func);
	if (dst == NULL) {
		return false;
	}

	if ( !xTimerStart(dst, 0) ) {
		return false;
	}

	return true;
}

void discovery_scheduler_periodic_func(const TimerHandle_t timer)
{
	/* Rescheudle ourself */
//	ctimer_reset(&dst);
	xTimerReset(dst, 0);

	/* Trigger discovery module to send a message */
	DISCOVERY.start(0, 0);
}

void discovery_scheduler_periodic_set_schedule_index(uint8_t index) {
	/* nop */
}

const struct discovery_scheduler_driver discovery_scheduler_periodic = {
		.init = discovery_scheduler_periodic_init,
		.set_schedule_index = discovery_scheduler_periodic_set_schedule_index,
};

/** @} */
/** @} */
