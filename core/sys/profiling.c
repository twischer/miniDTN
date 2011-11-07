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

#ifdef MAX_CONF_PROFILES
#define MAX_PROFILES MAX_CONF_PROFILES
#else
#define MAX_PROFILES 400
#endif /* MAX_CONF_PROFILES */

static struct profile_t profile;
static struct profile_site_t site[MAX_PROFILES];

/* We have to restrict instrumenting printf as well! */
void profiling_report(uint8_t pretty)
{
        int i;

	if (pretty)
		printf("PROF: %u sites\nfrom:to:calls:time\n", profile.num_sites);
	else
		printf("PROF:%u\n", profile.num_sites);

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
	profile.sites = site;
	profile.max_sites = MAX_PROFILES;
	profile.status = 0;
}

inline void profiling_start(void)
{
	profile.status |= PROFILING_STARTED;
}

inline void profiling_stop(void)
{
	profile.status &= ~PROFILING_STARTED;
}

/* Don't instrument the instrumentation functions */
void __cyg_profile_func_enter(void *, void *) __attribute__ ((no_instrument_function));
void __cyg_profile_func_exit(void *, void *) __attribute__ ((no_instrument_function));

/* XXX: What about recursion and nested recursion? */
void __cyg_profile_func_enter(void *func, void *caller)
{
      uint16_t i;

      if (!(profile.status&PROFILING_STARTED))
	      return;

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
                      profile.sites[i].time_start = 0;
		      profile.sites[i].time_accum = 0;
                      profile.num_sites++;
              } else {
                      profile.sites[i].calls++;
              }

	      /* Prevent infinite loop if clock_time() is not compiled with no_instrument_function */
	      profiling_stop();
              profile.sites[i].time_start = clock_time();
	      profiling_start();
      }
}

void __cyg_profile_func_exit(void *func, void *caller)
{
	uint16_t i;

      if (!(profile.status&PROFILING_STARTED))
	      return;

      for (i=0;i<profile.num_sites;i++) {
              if (profile.sites[i].addr == func &&
			      profile.sites[i].from == caller)
                      break;
      }
      if (i<profile.num_sites) {
	      profiling_stop();
              profile.sites[i].time_accum += (clock_time() - profile.sites[i].time_start)%65536;
	      profiling_start();
      }
}


