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
 * \brief PDS discovery schedule module
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

#ifndef DISCOVERY_TIMESLOT_LENGTH
#define DISCOVERY_TIMESLOT_LENGTH 1
#endif

#define PDS_SCHED_2_1 21
#define PDS_SCHED_2_2 22
#define PDS_SCHED_2_3 23
#define PDS_SCHED_3_2 32
#define PDS_SCHED_2_4 24
#define PDS_SCHED_7_2 72
#define PDS_SCHED_DUMMY 999

#ifndef PDS_SCHED
#define PDS_SCHED PDS_SCHED_2_1
#endif

PROCESS(discovery_scheduler_pds_process, "DISCOVERY_SCHEDULER_PDS process");

static struct etimer discovery_scheduler_timer;

void discovery_scheduler_pds_init() {
	// Start discovery scheduler process
	process_start(&discovery_scheduler_pds_process, NULL);
}

PROCESS_THREAD(discovery_scheduler_pds_process, ev, data)
{
#if PDS_SCHED == PDS_SCHED_2_1
	/* pds2^1={0,1,3} schedule, length 7, dutycycle ~43%, equivalent to {1,1,0,1,0,0,0} */
	static int sched[] = {2,1,1,3};
	int schedule_length = 4;
#elif PDS_SCHED == PDS_SCHED_2_2
	/* pds2^2={0,1,4,14,16} schedule, length 21, dutycycle ~24%, equivalent to {1,1,0,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0} */
	static int sched[] = {2,2,1,9,1,1,1,4};
	int schedule_length = 8;
#elif PDS_SCHED == PDS_SCHED_2_3
	/* pds2^3={0,1,3,7,15,31,36,54,63} schedule, length 73, dutycycle ~12% */
	static int sched[] = { 2, 1, 1, 3, 1, 7, 1, 15, 1, 4, 1, 17, 1, 8, 1, 9 };
	int schedule_length = 16;
#elif PDS_SCHED == PDS_SCHED_2_4
	/* pds2^4={...} schedule, length 273, dutycycle ~6% */
	static int sched[] = { 2, 1, 1, 3, 1, 7, 1, 15, 1, 31, 1, 26, 1, 25, 1, 10, 1, 8, 1, 44, 1, 12, 1, 9, 1, 28, 1, 4, 1, 16, 1, 17 };
	int schedule_length = 32;
#elif PDS_SCHED == PDS_SCHED_3_2
  /* pds3^2={...} schedule, length 91, dutycycle ~10% */
  static int sched[] = { 2, 1, 1, 5, 1, 17, 1, 21, 1, 6, 1, 4, 1, 15, 1, 3, 1, 9 };
  int schedule_length = 18;
#elif PDS_SCHED == PDS_SCHED_7_2
	/* pds7^2={...} schedule, length 2451, dutycycle ~2% */
	static int sched[] = { 2, 5, 1, 41, 1, 120, 1, 52, 1, 43, 1, 75, 1, 4, 1, 33, 1, 108, 1, 16, 1, 179, 1, 61, 1, 77, 1, 57, 1, 7, 1, 82, 1, 8, 1, 2, 1, 115, 1, 17, 1, 53, 1, 12, 1, 109, 1, 3, 1, 51, 1, 204, 1, 72, 1, 81, 1, 27, 1, 2, 1, 27, 1, 73, 1, 19, 1, 22, 1, 44, 1, 62, 1, 20, 1, 79, 1, 37, 1, 31, 1, 36, 1, 60, 1, 76, 1, 18, 1, 35, 1, 9, 1, 13, 1, 10, 1, 14 };
	int schedule_length = 98;
#elif PDS_SCHED == PDS_SCHED_DUMMY
	static int sched[] = {320,1};
	int schedule_length = 2;
#endif

	static int schedule_index = 0;
#ifdef PDS_OFFSET
	schedule_index = PDS_OFFSET;
#endif

	/* schedule always starts with discovery on. state is switched at beginning
	 * of loop, therefore set to zero, here */
	static int sched_state = 0;

	PROCESS_BEGIN();

	etimer_set(&discovery_scheduler_timer, DISCOVERY_TIMESLOT_LENGTH * CLOCK_SECOND);
	LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "PDS Discovery scheduler process starting with initial offset %u", schedule_index);

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&discovery_scheduler_timer));

		/* calculate new timeout */
		int newTimeout = sched[schedule_index] * DISCOVERY_TIMESLOT_LENGTH * CLOCK_SECOND;
		schedule_index++;

		/* wrap schedule if appropriate */
		schedule_index = schedule_index % schedule_length;

		/* switch discovery state */
		sched_state = ~sched_state;

		if (sched_state) {
			LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "DISCOVERY SCHEDULER PDS: begin of discovery phase");
			//process_post(PROCESS_BROADCAST, 0xA2, 0);
			process_post_synch(&discovery_aware_rdc_process, dtn_disco_start_event , 0);
			DISCOVERY.start(newTimeout);
		} else {
			LOG(LOGD_DTN, LOG_DISCOVERY_SCHEDULER, LOGL_DBG, "DISCOVERY SCHEDULER PDS: end of discovery phase");
			//process_post(PROCESS_BROADCAST, 0xA3, 0);
			process_post_synch(&discovery_aware_rdc_process, dtn_disco_stop_event, 0);
			DISCOVERY.stop();
		}



		/* set timer */
		etimer_set(&discovery_scheduler_timer, newTimeout);

	}
	PROCESS_END();
}

const struct discovery_scheduler_driver discovery_scheduler_pds = {
		.init = discovery_scheduler_pds_init,
};

/** @} */
/** @} */
