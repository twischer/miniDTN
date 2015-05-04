/**
 * \addtogroup discovery
 * @{
 */

/**
 * \defgroup discovery_scheduler_null Always on discovery scheduler module
 *
 * @{
 */

/**
 * \file
 * \brief Always on discovery schedule module
 *
 * \author André Frambach <frambach@ibr.cs.tu-bs.de>
 */

#include "agent.h"
#include "discovery_scheduler.h"
#include "discovery_scheduler_events.h"
#include "discovery.h"

#include "net/mac/discovery_aware_rdc.h"


void discovery_scheduler_null_init() {
  process_post(&discovery_aware_rdc_process, dtn_disco_start_event, 0);
  DISCOVERY.start(0, 0);
}

const struct discovery_scheduler_driver discovery_scheduler_null = {
    .init = discovery_scheduler_null_init,
};

/** @} */
/** @} */
