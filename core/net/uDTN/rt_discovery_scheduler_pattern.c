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
#include "rtimer.h"
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
#define DTN_DISCO_PATTERN {2,1,1,3}
#endif


static struct rtimer t;
void rt_discovery_scheduler_pattern_func(struct rtimer *t, void *ptr);

void rt_discovery_scheduler_pattern_init() {
  // schedule ourself...
  rtimer_set(&t, RTIMER_TIME(&t) + DTN_DISCO_TIMESLOT_LENGTH * RTIMER_ARCH_SECOND, 1,
      rt_discovery_scheduler_pattern_func, NULL);
}


void rt_discovery_scheduler_pattern_func(struct rtimer *t, void *ptr)
{
  static uint8_t schedule[] = DTN_DISCO_PATTERN;
  static uint8_t schedule_index = 0;
  static uint8_t schedule_length = sizeof(schedule) / sizeof(schedule[0]);

  /* schedule always starts with discovery on. state is switched at beginning
   * of loop, therefore set to zero, here */
  static uint8_t sched_state = 0;


  /* calculate new timeout */
  uint16_t newTimeout =  RTIMER_ARCH_SECOND * 1.0 * (schedule[schedule_index] * DTN_DISCO_TIMESLOT_LENGTH);
  //printf("PATTERN DISCOVERY SCHEDULER: Timeout   schedule[schedule_index]: %u\n", schedule[schedule_index]);
  //printf("PATTERN DISCOVERY SCHEDULER: ARCH SECOND Timeout: %u\n", RTIMER_ARCH_SECOND);
  schedule_index++;

  /* wrap schedule if appropriate */
  schedule_index = schedule_index % schedule_length;

  /* switch discovery state */
  sched_state = ~sched_state;

  if (sched_state) {
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "PATTERN DISCOVERY SCHEDULER: begin of discovery phase");
    process_post_synch(&discovery_aware_rdc_process, dtn_disco_start_event, 0);
    DISCOVERY.start(newTimeout, schedule_index);
  } else {
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "PATTERN DISCOVERY SCHEDULER: end of discovery phase");
    DISCOVERY.stop();
    process_post_synch(&discovery_aware_rdc_process, dtn_disco_stop_event, 0);
  }



  /* reschedule outself */
  //etimer_set(&discovery_scheduler_timer, newTimeout);

  //printf("PATTERN DISCOVERY SCHEDULER: New Timeout: %d\n", newTimeout);
  uint16_t to = RTIMER_NOW() + newTimeout;
  int r = rtimer_set(t, to, 1, rt_discovery_scheduler_pattern_func, NULL);
  if(r != RTIMER_OK) {
       printf("PATTERN DISCOVERY SCHEDULER: could not set rtimer\n");
  }

  //printf("PATTERN DISCOVERY SCHEDULER: Timeout: %d\n", to);
}


const struct discovery_scheduler_driver rt_discovery_scheduler_pattern = {
		.init = rt_discovery_scheduler_pattern_init,
};

/** @} */
/** @} */
