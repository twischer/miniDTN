/*
 * Copyright (c) 2011, Daniel Willmann <daniel@totalueberwachung.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Compiler assisted profiler for Contiki - needs gcc -finstrument-functions
 *
 * @(#)$$
 */

#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "sys/profiling.h"
#include "profiling_arch.h"

/* Disable by default */
#ifdef PROFILES_CONF_MAX
#define MAX_PROFILES PROFILES_CONF_MAX
#else
#define MAX_PROFILES 0
#endif /* MAX_CONF_PROFILES */

/* Disable by default */
#ifdef PROFILES_CONF_STACKSIZE
#define PROFILE_STACKSIZE PROFILES_CONF_STACKSIZE
#else
#define PROFILE_STACKSIZE 0
#endif /* PROFILES_CONF_STACKSIZE */

static struct profile_t profile;
static struct profile_site_t site[MAX_PROFILES];
static int fine_count;

static struct profile_callstack_t callstack[PROFILE_STACKSIZE] __attribute__ ((section (".noinit")));
static int stacklevel __attribute__ ((section (".noinit")));

void profiling_stack_trace(void)
{
	int i;
	if ((stacklevel <= 0) || (stacklevel > PROFILE_STACKSIZE)) {
		printf("Stack corrupted or not recorded.\n");
		return;
	}

	printf("Stacktrace: %i frames, %lu ticks/s\n", stacklevel, CLOCK_SECOND*256l);

	for (i=0; i<stacklevel; i++) {
		printf("%i: %p->%p @%lu\n", i, ARCHADDR2ADDR(callstack[i].caller), ARCHADDR2ADDR(callstack[i].func), callstack[i].time_start);
	}
	printf("\n");
}

static inline void profiling_internal(uint8_t internal)  __attribute__ ((no_instrument_function));
static inline void profiling_internal(uint8_t internal)
{
	if (internal)
		profile.status |= PROFILING_INTERNAL;
	else
		profile.status &= ~PROFILING_INTERNAL;
}

void profiling_report(const char *name, uint8_t pretty)
{
        int i;

	/* The parser would be confused if the name contains colons or newlines, so disallow */
	if (!name || strchr(name, ':') || strchr(name, '\r') || strchr(name, '\n')) {
		printf("The profile report name is invalid\n");
		name = "invalid";
	}

	if (pretty)
		printf("PROF: \"%s\" %u sites %u max sites %lu ticks spent %lu ticks/s\nfrom:to:calls:time\n", name, profile.num_sites, profile.max_sites, profile.time_run, CLOCK_SECOND*256l);
	else
		printf("PROF:%s:%u:%u:%lu:%lu\n", name, profile.num_sites, profile.max_sites, profile.time_run, CLOCK_SECOND*256l);

	for(i=0; i<profile.num_sites;i++) {
		printf("%p:%p:%lu:%lu:%u:%u\n", ARCHADDR2ADDR(profile.sites[i].from), ARCHADDR2ADDR(profile.sites[i].addr),
				profile.sites[i].calls, profile.sites[i].time_accum, profile.sites[i].time_min, profile.sites[i].time_max);
	}
	printf("\n");
}

struct profile_t *profiling_get()
{
	return &profile;
}

void profiling_init(void)
{
	fine_count = clock_fine_max() + 1;

	profile.sites = site;
	profile.max_sites = MAX_PROFILES;
	profile.num_sites = 0;
	profile.time_run = 0;
	profile.status = 0;
	stacklevel = 0;
}

void profiling_start(void)
{
	clock_time_t now;
	unsigned short now_fine;

	if (profile.status & PROFILING_STARTED)
		return;

	do {
		now_fine = clock_fine();
		now = clock_time();
	} while (now_fine != clock_fine());


	profile.time_start = ((unsigned long)now<<8) + now_fine*256/fine_count;
	profile.status |= PROFILING_STARTED;

	/* Reset the callstack */
	stacklevel = 0;
}

void profiling_stop(void)
{
	unsigned long temp;
	clock_time_t now;
	unsigned short now_fine;

	if (!profile.status & PROFILING_STARTED)
		return;

	do {
		now_fine = clock_fine();
		now = clock_time();
	} while (now_fine != clock_fine());


	temp = ((unsigned long)now<<8) + now_fine*256/fine_count;

	profile.time_run += (temp - profile.time_start);
	profile.status &= ~PROFILING_STARTED;
}

/* Don't instrument the instrumentation functions */
void __cyg_profile_func_enter(void *, void *) __attribute__ ((no_instrument_function));
void __cyg_profile_func_exit(void *, void *) __attribute__ ((no_instrument_function));
static inline struct profile_site_t *find_or_add_site(void *func, void *caller, uint8_t findonly)  __attribute__ ((no_instrument_function));

static inline struct profile_site_t *find_or_add_site(void *func, void *caller, uint8_t findonly)
{
	struct profile_site_t *site = NULL;
	int16_t lower, upper;
	uint16_t newindex = 0, i;

	lower = 0;
	upper = profile.num_sites-1;

	while (lower <= upper) {
		newindex = ((upper - lower)>>1) + lower;
		site = &profile.sites[newindex];
		if (func > site->addr) {
			lower = newindex + 1;
		} else if (func < site->addr) {
			upper = newindex - 1;
		} else {
			if (caller > site->from) {
				lower = newindex + 1;
			} else if (caller < site->from) {
				upper = newindex - 1;
			} else {
				return site;
			}
		}
	}

	/* We only want to get an entry if it existed */
	if (findonly)
		return NULL;

	/* Table is full - nothing we can do */
	if (profile.num_sites == profile.max_sites)
		return NULL;

	newindex = lower;
	/* Site not found, need to insert it */
	for (i = profile.num_sites; i>newindex; i--) {
		memcpy(&profile.sites[i], &profile.sites[i-1], sizeof(struct profile_site_t));
	}
	profile.num_sites++;
	site = &profile.sites[newindex];
	site->from = caller;
	site->addr = func;
	site->calls = 0;
	site->time_max = 0;
	site->time_min = 0xFFFF;
	site->time_accum = 0;

	return site;
}

void __cyg_profile_func_enter(void *func, void *caller)
{
      struct profile_site_t *site;
      clock_time_t now;
      unsigned short now_fine;

      if (!(profile.status&PROFILING_STARTED) || (profile.status&PROFILING_INTERNAL))
	      return;

      profiling_internal(1);

      if (stacklevel >= PROFILE_STACKSIZE)
	      goto out;

      site = find_or_add_site(func, caller, 0);
      if (!site)
	      goto out;

      /* Update the call stack */
      do {
	      now_fine = clock_fine();
	      now = clock_time();
      } while (now_fine != clock_fine());

      callstack[stacklevel].time_start = ((unsigned long)now<<8) + now_fine*256/fine_count;
      callstack[stacklevel].func = func;
      callstack[stacklevel].caller = caller;

      stacklevel++;

out:
      profiling_internal(0);
}

void __cyg_profile_func_exit(void *func, void *caller)
{
	unsigned long temp;
	struct profile_site_t *site;
	clock_time_t now;
	unsigned short now_fine;

      if (!(profile.status&PROFILING_STARTED) || (profile.status&PROFILING_INTERNAL))
	      return;

      profiling_internal(1);

      do {
	      now_fine = clock_fine();
	      now = clock_time();
      } while (now_fine != clock_fine());

      temp = ((unsigned long)now<<8) + now_fine*256/fine_count;

      /* See if this call was recorded on the call stack */
      if (stacklevel > 0 && callstack[stacklevel-1].func == func &&
		      callstack[stacklevel-1].caller == caller) {
	      temp = temp - callstack[stacklevel-1].time_start;
	      stacklevel--;
      } else {
	      goto out;
      }

      site = find_or_add_site(func, caller, 1);
      if (!site)
	      goto out;

      /* Update calls and time */
      site->calls++;
      site->time_accum += temp;

      /* Min max calculation */
      if (temp > site->time_max) {
	      site->time_max = temp;
	      if (temp > 0xFFFF)
		      site->time_max = 0xFFFF;
      }
      if (temp < site->time_min)
	      site->time_min = temp;

      profiling_internal(0);
      return;

out:
      /* We didn't find the entry, abort */
      profiling_internal(0);
}


