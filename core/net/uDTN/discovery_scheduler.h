/*
 * discovery_scheduler.h
 *
 *  Created on: 28.01.2013
 *      Author: andre
 */

#ifndef DISCOVERY_SCHEDULER_H_
#define DISCOVERY_SCHEDULER_H_

#include <stdbool.h>

#include "net/rime/rime.h"

/**
 * Which discovery scheduler driver are we going to use?
 */
#ifdef CONF_DISCOVERY_SCHEDULER
#define DISCOVERY_SCHEDULER CONF_DISCOVERY_SCHEDULER
#else
#define DISCOVERY_SCHEDULER discovery_scheduler_periodic
#endif

/**
 * How often should we send discovery messages?
 */
#ifdef DISCOVERY_CONF_CYCLE
#define DISCOVERY_CYCLE DISCOVERY_CONF_CYCLE
#else
#define DISCOVERY_CYCLE 1
#endif

/** interface for discovery scheduler modules **/
struct discovery_scheduler_driver {
	bool (* init)();
	void (* set_schedule_index)(uint8_t index);
};

extern const struct discovery_scheduler_driver DISCOVERY_SCHEDULER;

#endif /* DISCOVERY_SCHEDULER_H_ */
