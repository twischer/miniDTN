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
 * \author Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */
#include "discovery_scheduler.h"
#include "discovery.h"

#include "sys/ctimer.h"

/**
 * How often should we send discovery messages?
 */
#ifdef DISCOVERY_CONF_CYCLE
#define DISCOVERY_CYCLE DISCOVERY_CONF_CYCLE
#else
#define DISCOVERY_CYCLE 5
#endif

static struct ctimer dst;

void discovery_scheduler_periodic_func(void * ptr);

void discovery_scheduler_periodic_init() {
	ctimer_set(&dst, DISCOVERY_CYCLE * CLOCK_SECOND, discovery_scheduler_periodic_func, NULL);
}

void discovery_scheduler_periodic_func(void * ptr)
{
	/* Rescheudle ourself */
	ctimer_reset(&dst);

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
