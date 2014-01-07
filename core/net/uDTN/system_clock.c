/*
 * clock.c
 *
 *  Created on: 17.12.2013
 *      Author: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 */

#include "system_clock.h"

static udtn_timeval_t udtn_clock_offset;
static udtn_clock_state_t udtn_clock_state;

void udtn_clock_init() {
	// set the clock offset to zero
	udtn_timerclear(&udtn_clock_offset);

	// set initial clock state
	udtn_setclockstate(UDTN_CLOCK_STATE_UNKNOWN);
}

udtn_time_t udtn_time(udtn_time_t *t) {
	udtn_timeval_t ret;
	udtn_gettimeofday(&ret);

	if (t != NULL) {
		(*t) = ret.tv_sec;
		return 0;
	}
	return ret.tv_sec;
}

void udtn_settimeofday(udtn_timeval_t *tv) {
	udtn_timeval_t ref;

	// get system uptime
	udtn_uptime(&ref);

	// set global clock offset
	udtn_timersub(tv, &ref, &udtn_clock_offset);
}

void udtn_gettimeofday(udtn_timeval_t *tv) {
	udtn_timeval_t ref;

	// get system uptime
	udtn_uptime(&ref);

	// add global clock offset
	udtn_timeradd(&ref, &udtn_clock_offset, tv);
}

void udtn_uptime(udtn_timeval_t *tv) {
	do {
		tv->tv_usec = (uint32_t)clock_time();
		tv->tv_sec = (uint32_t)clock_seconds();
	} while (tv->tv_usec != (uint32_t)clock_time());

	// convert to microseconds
	tv->tv_usec = ((tv->tv_usec % CLOCK_SECOND) * 1000000) / CLOCK_SECOND;
}

void udtn_setclockstate(udtn_clock_state_t s) {
	udtn_clock_state = s;
}

udtn_clock_state_t udtn_getclockstate() {
	return udtn_clock_state;
}
