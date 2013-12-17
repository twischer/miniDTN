/*
 * clock.c
 *
 *  Created on: 17.12.2013
 *      Author: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 */

#include "system_clock.h"

static struct udtn_timeval udtn_clock_offset;

void udtn_clock_init() {
	// set the clock offset to zero
	udtn_timerclear(&udtn_clock_offset);
}

udtn_time_t udtn_time(udtn_time_t *t) {
	struct udtn_timeval ret;
	udtn_gettimeofday(&ret);

	if (t != NULL) {
		(*t) = ret.tv_sec;
		return 0;
	}
	return ret.tv_sec;
}

void udtn_settimeofday(struct udtn_timeval *tv) {
	struct udtn_timeval ref;

	// get system uptime
	udtn_uptime(&ref);

	// set global clock offset
	udtn_timersub(tv, &ref, &udtn_clock_offset);
}

void udtn_gettimeofday(struct udtn_timeval *tv) {
	struct udtn_timeval ref;

	// get system uptime
	udtn_uptime(&ref);

	// add global clock offset
	udtn_timeradd(&ref, &udtn_clock_offset, tv);
}

void udtn_uptime(struct udtn_timeval *tv) {
	do {
		tv->tv_usec = (uint32_t)clock_time();
		tv->tv_sec = (uint32_t)clock_seconds();
	} while (tv->tv_usec != (uint32_t)clock_time());

	// convert to microseconds
	tv->tv_usec = ((tv->tv_usec % CLOCK_SECOND) * 1000000) / CLOCK_SECOND;
}
