/**
 * \addtogroup discovery
 * @{
 */

/**
 * \defgroup discovery_pds PDS discovery module
 *
 * @{
 */

/**
 * \file
 * \brief Simple discovery schedule module
 *
 * \author André Frambach <frambach@ibr.cs.tu-bs.de>
 */

#include "logging.h"
#include "agent.h"
#include "discovery_scheduler.h"
#include "discovery.h"

#define DISCOVERY_SIMPLE_PERIOD 100


PROCESS(discovery_scheduler_simple_process, "DISCOVERY_SCHEDULER_SIMPLE process");

static struct etimer discovery_scheduler_timer;

void discovery_scheduler_simple_init() {
	// Start discovery scheduler process
	process_start(&discovery_scheduler_simple_process, NULL);
}

PROCESS_THREAD(discovery_scheduler_simple_process, ev, data)
{
	static int sched_state = 0;

	PROCESS_BEGIN();

	etimer_set(&discovery_scheduler_timer, DISCOVERY_SIMPLE_PERIOD * CLOCK_SECOND);
	LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "Simple Discovery scheduler process starting.");


	while(1) {
		PROCESS_WAIT_EVENT();

		if( etimer_expired(&discovery_scheduler_timer) ) {

			/* switch discovery state */
			sched_state = ~sched_state;

			if (sched_state) {
				LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "DISCOVERY SCHEDULER SIMPLE: begin of discovery phase");
				process_post(PROCESS_BROADCAST, 0xA2, 0);
				DISCOVERY.start(DISCOVERY_SIMPLE_PERIOD * CLOCK_SECOND, 0);
			} else {
				LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "DISCOVERY SCHEDULER SIMPLE: end of discovery phase");
				process_post(PROCESS_BROADCAST, 0xA3, 0);
				DISCOVERY.stop();
			}

			/* set timer */
			etimer_reset(&discovery_scheduler_timer);
		}

	}
	PROCESS_END();
}

const struct discovery_scheduler_driver discovery_scheduler_simple = {
		.init = discovery_scheduler_simple_init,
};

/** @} */
/** @} */
