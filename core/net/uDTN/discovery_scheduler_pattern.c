/**
 * \addtogroup discovery
 * @{
 */

/**
 * \defgroup discovery_pattern Pattern discovery scheduler module
 *
 * @{
 */

/**
 * \file
 * \brief Pattern discovery scheduler module
 *
 * \author André Frambach <frambach@ibr.cs.tu-bs.de>
 */
#include "logging.h"
#include "agent.h"
#include "discovery_scheduler.h"
#include "discovery_scheduler_events.h"
#include "discovery.h"

#include "net/netstack.h"
#include "net/mac/discovery_aware_rdc.h"

/**
 * Discovery Timeslot Length in Seconds
 */
#ifdef DTN_DISCO_TIMESLOT_LENGTH_CONF
#define DTN_DISCO_TIMESLOT_LENGTH DTN_DISCO_TIMESLOT_LENGTH_CONF
#else
#define DTN_DISCO_TIMESLOT_LENGTH 1
#endif


/**
 * Discovery Pattern. Default is 1sec on, 1sec off.
 */
#ifdef DTN_DISCO_PATTERN_CONF
#define DTN_DISCO_PATTERN DTN_DISCO_PATTERN_CONF
#else
#define DTN_DISCO_PATTERN {1,1}
#endif

PROCESS(discovery_scheduler_pattern_process, "DISCOVERY_SCHEDULER_PATTERN process");

static struct etimer discovery_scheduler_timer;

void discovery_scheduler_pattern_init() {
	// Start discovery scheduler process
	process_start(&discovery_scheduler_pattern_process, NULL);
}

PROCESS_THREAD(discovery_scheduler_pattern_process, ev, data)
{
	static uint8_t schedule[] = DTN_DISCO_PATTERN;
	static uint8_t schedule_index = 0;
	static uint8_t schedule_length = sizeof(schedule) / sizeof(schedule[0]);

	/* schedule always starts with discovery on. state is switched at beginning
	 * of loop, therefore set to zero, here */
	static uint8_t sched_state = 0;

	PROCESS_BEGIN();

	etimer_set(&discovery_scheduler_timer, DTN_DISCO_TIMESLOT_LENGTH * CLOCK_SECOND);
	LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "PATTERN DISCOVERY SCHEDULER: process starting");

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&discovery_scheduler_timer));

		/* switch discovery state */
		sched_state = ~sched_state;

		if (sched_state) {
			LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "PATTERN DISCOVERY SCHEDULER: begin of discovery phase");
			process_post(&discovery_aware_rdc_process,dtn_disco_start_event , 0);
			DISCOVERY.start();
		} else {
			LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "PATTERN DISCOVERY SCHEDULER: end of discovery phase");
			process_post(&discovery_aware_rdc_process, dtn_disco_stop_event, 0);
			DISCOVERY.stop();
		}

		/* calculate new timeout */
		int newTimeout = schedule[schedule_index] * DTN_DISCO_TIMESLOT_LENGTH * CLOCK_SECOND;
		schedule_index++;

		/* wrap schedule if appropriate */
		schedule_index = schedule_index % schedule_length;

		/* set timer */
		etimer_set(&discovery_scheduler_timer, newTimeout);

	}
	PROCESS_END();
}

const struct discovery_scheduler_driver discovery_scheduler_pattern = {
		.init = discovery_scheduler_pattern_init,
};

/** @} */
/** @} */
