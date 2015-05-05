#ifndef __PROFILING_H__
#define __PROFILING_H__

#include <stdint.h>

#define PROFILING_STARTED 1
#define PROFILING_INTERNAL 2

struct profile_callstack_t {
	void *func;
	void *caller;
	unsigned long time_start;
};

/* The structure that holds the callsites */
struct profile_site_t {
	void *from;
	void *addr;
	uint32_t calls;
	uint16_t time_min;
	uint16_t time_max;
	unsigned long time_accum;
};

struct profile_t {
	int status;
	uint16_t max_sites;
	uint16_t num_sites;
	unsigned long time_run;
	unsigned long time_start;
	struct profile_site_t *sites;
};



void profiling_init(void) __attribute__ ((no_instrument_function));
void profiling_start(void) __attribute__ ((no_instrument_function));
void profiling_stop(void) __attribute__ ((no_instrument_function));
void profiling_report(const char *name, uint8_t pretty) __attribute__ ((no_instrument_function));
struct profile_t *profiling_get(void) __attribute__ ((no_instrument_function));
void profiling_stack_trace(void) __attribute__ ((no_instrument_function));

#endif /* __PROFILING_H__ */
