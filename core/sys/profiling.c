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

#ifdef PROFILES_CONF_MAX
#define MAX_PROFILES PROFILES_CONF_MAX
#else
#define MAX_PROFILES 100
#endif /* MAX_CONF_PROFILES */

#ifdef PROFILES_CONF_STACKSIZE
#define PROFILE_STACKSIZE PROFILES_CONF_STACKSIZE
#else
#define PROFILE_STACKSIZE 20
#endif /* PROFILES_CONF_STACKSIZE */

static struct profile_t profile;
static struct profile_site_t site[MAX_PROFILES];
static int fine_count;

static struct profile_callstack_t callstack[PROFILE_STACKSIZE];
static int stacklevel;

static inline void profiling_internal(uint8_t internal)  __attribute__ ((no_instrument_function));
static inline void profiling_internal(uint8_t internal)
{
	if (internal)
		profile.status |= PROFILING_INTERNAL;
	else
		profile.status &= ~PROFILING_INTERNAL;
}

void profiling_report(uint8_t pretty)
{
        int i;

	if (pretty)
		printf("PROF: %u sites %u max sites %lu ticks spent %lu ticks/s\nfrom:to:calls:time\n", profile.num_sites, profile.max_sites, profile.time_run, CLOCK_SECOND*256l);
	else
		printf("PROF:%u:%u:%lu:%lu\n", profile.num_sites, profile.max_sites, profile.time_run, CLOCK_SECOND*256l);

	for(i=0; i<profile.num_sites;i++) {
		printf("%p:%p:%lu:%lu\n", profile.sites[i].from, profile.sites[i].addr,
				profile.sites[i].calls, profile.sites[i].time_accum);
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
	profile.time_run = 0;
	profile.status = 0;
	stacklevel = 0;
}

void profiling_start(void)
{
	if (profile.status & PROFILING_STARTED)
		return;

	profile.time_start = ((unsigned long)clock_time()<<8) + clock_fine()*256/fine_count;
	profile.status |= PROFILING_STARTED;

	/* Reset the callstack */
	stacklevel = 0;
}

void profiling_stop(void)
{
	unsigned long temp;

	if (!profile.status & PROFILING_STARTED)
		return;

	temp = ((unsigned long)clock_time()<<8) + clock_fine()*256/fine_count;

	profile.time_run += (temp - profile.time_start);
	profile.status &= ~PROFILING_STARTED;
}

/* Don't instrument the instrumentation functions */
void __cyg_profile_func_enter(void *, void *) __attribute__ ((no_instrument_function));
void __cyg_profile_func_exit(void *, void *) __attribute__ ((no_instrument_function));

void __cyg_profile_func_enter(void *func, void *caller)
{
      uint16_t i;

      if (!(profile.status&PROFILING_STARTED) || (profile.status&PROFILING_INTERNAL))
	      return;

      profiling_internal(1);

      if (stacklevel >= PROFILE_STACKSIZE)
	      goto out;

      for (i=0;i<profile.num_sites;i++) {
              if (profile.sites[i].addr == func &&
			      profile.sites[i].from == caller)
                      break;
      }
      if (i<profile.max_sites) {
              /* XXX: Possible race for interrupts here? Disable? */
              if (i==profile.num_sites) {
                      profile.sites[i].addr = func ;
		      profile.sites[i].from = caller;
		      profile.sites[i].calls = 1;
		      profile.sites[i].time_accum = 0;
                      profile.num_sites++;
              } else {
                      profile.sites[i].calls++;
              }

              /* Update the call stack */
              callstack[stacklevel].time_start = ((unsigned long)clock_time()<<8) + clock_fine()*256/fine_count;
              callstack[stacklevel].func = func;
              callstack[stacklevel].caller = caller;

              stacklevel++;
      }

out:
      profiling_internal(0);
}

void __cyg_profile_func_exit(void *func, void *caller)
{
	uint16_t i;
	unsigned long temp;
	struct profile_site_t *site;

      if (!(profile.status&PROFILING_STARTED) || (profile.status&PROFILING_INTERNAL))
	      return;

      profiling_internal(1);
      temp = ((unsigned long)clock_time()<<8) + clock_fine()*256/fine_count;

      /* See if this call was recorded on the call stack */
      if (stacklevel > 0 && callstack[stacklevel-1].func == func &&
		      callstack[stacklevel-1].caller == caller) {
	      temp = temp - callstack[stacklevel-1].time_start;
	      stacklevel--;
      } else {
	      goto out;
      }

      for (i=0;i<profile.num_sites;i++) {
              if (profile.sites[i].addr == func &&
			      profile.sites[i].from == caller) {
		      site = &profile.sites[i];
	      }
      }
      if (!site)
	      goto out;

      /* Update calls and time */
      site->calls++;
      site->time_accum += temp;
      profiling_internal(0);
      return;

out:
      /* We didn't find the entry, abort */
      profiling_internal(0);
}


