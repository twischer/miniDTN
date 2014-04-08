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
 * Statistical profiler for Contiki - it will not be able to profile interrupts
 * or parts where interrupts are disabled.
 *
 * @(#)$$
 */

#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "sys/sprofiling.h"

#ifdef SPROFILES_CONF_MAX
#define MAX_PROFILES SPROFILES_CONF_MAX
#else
#define MAX_PROFILES 1
#endif /* MAX_CONF_PROFILES */

static struct sprofile_t stat_profile;
static struct sprofile_site_t stat_site[MAX_PROFILES];

void sprofiling_report(const char* name, uint8_t pretty)
{
	int i;

	/* The parser would be confused if the name contains colons or newlines, so disallow */
	if (!name || strchr(name, ':') || strchr(name, '\r') || strchr(name, '\n')) {
		printf("The profile report name is invalid\n");
		name = "invalid";
	}

	if (pretty)
		printf("\nSPROF: \"%s\" %u sites %u max_sites %lu samples\npc:calls\n", name, stat_profile.num_sites, stat_profile.max_sites, stat_profile.num_samples);
	else
		printf("\nSPROF:%s:%u:%u:%lu\n", name, stat_profile.num_sites, stat_profile.max_sites, stat_profile.num_samples);

	for(i=0; i<stat_profile.num_sites;i++) {
		printf("%p:%u\n", stat_profile.sites[i].addr, stat_profile.sites[i].calls);
	}
}

struct sprofile_t *sprofiling_get()
{
	return &stat_profile;
}

inline void sprofiling_add_sample(void *pc)
{
        uint8_t i;
        for (i=0;i<stat_profile.num_sites;i++) {
                if (stat_profile.sites[i].addr == pc)
                        break;
        }
        if (i<stat_profile.max_sites) {
                if (i==stat_profile.num_sites) {
                        stat_profile.sites[i].addr = pc;
                        stat_profile.sites[i].calls = 1;
                        stat_profile.num_sites++;
                } else {
                        stat_profile.sites[i].calls++;
                }
		stat_profile.num_samples++;
        }
}

void sprofiling_init(void)
{
	stat_profile.sites = stat_site;
	stat_profile.max_sites = MAX_PROFILES;

	sprofiling_arch_init();
}

inline void sprofiling_start(void)
{
	sprofiling_arch_start();
}

inline void sprofiling_stop(void)
{
	sprofiling_arch_stop();
}
