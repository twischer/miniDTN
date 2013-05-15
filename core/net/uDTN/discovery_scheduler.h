/*
 * discovery_scheduler.h
 *
 *  Created on: 28.01.2013
 *      Author: andre
 */

#ifndef DISCOVERY_SCHEDULER_H_
#define DISCOVERY_SCHEDULER_H_

#include "contiki.h"
#include "rime.h"
#include "process.h"

/**
 * Which discovery scheduler driver are we going to use?
 */
#ifdef CONF_DISCOVERY_SCHEDULER
#define DISCOVERY_SCHEDULER CONF_DISCOVERY_SCHEDULER
#else
#define DISCOVERY_SCHEDULER discovery_scheduler_pds
#endif


/** interface for discovery scheduler modules **/
struct discovery_scheduler_driver {
	void (* init)();
};

extern const struct discovery_scheduler_driver DISCOVERY_SCHEDULER;

#endif /* DISCOVERY_SCHEDULER_H_ */
