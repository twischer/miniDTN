/*
 * Copyright (c) 2012, Daniel Willmann <daniel@totaluberwachung.de>
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
 * $Id:$
 */

/**
 * \file
 *         Generate new bundles as quickly as possible
 * \author
 *         Daniel Willmann <daniel@totalueberwachung.de>
 */

#include "contiki.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "net/netstack.h"
#include "net/packetbuf.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/API_events.h"
#include "net/uDTN/API_registration.h"

#include "net/uDTN/dtn_config.h"
#include "net/uDTN/storage.h"
#include "mmem.h"
#include "sys/profiling.h"
#include "sys/test.h"
#include "watchdog.h"

#ifndef DATASIZE
#define DATASIZE 30
#endif

/*---------------------------------------------------------------------------*/
PROCESS(bundle_generator_process, "Bundle generator process");
PROCESS(profiling_process, "Profiling process");
AUTOSTART_PROCESSES(&profiling_process);
/*---------------------------------------------------------------------------*/
static uint32_t numbundles;

PROCESS_THREAD(profiling_process, ev, data)
{
	static struct etimer timer;
	PROCESS_BEGIN();
	agent_init();
	etimer_set(&timer, CLOCK_SECOND*1);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));
	printf_P(PSTR("Starting bundle generation\n"));
	profiling_init();
	profiling_start();
	process_start(&bundle_generator_process, NULL);
	etimer_set(&timer, CLOCK_SECOND*30);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));
	profiling_stop();
	watchdog_stop();
	profiling_report("bundle-generation", 0);
	TEST_REPORT("bundle-gen", numbundles, 30000, "kbundles/s");
	TEST_PASS();
	PROCESS_END();
}

PROCESS_THREAD(bundle_generator_process, ev, data)
{
	uint32_t val;
	static struct bundle_t bundle;
	static uint8_t databuf[DATASIZE];
	int i;
	for (i=0; i<DATASIZE; i++) {
		databuf[i] = i;
	}

	PROCESS_BEGIN();

	while(1) {
		PROCESS_PAUSE();

		create_bundle(&bundle);
		/* Set node destination and source address */ 
		val=0x0001;
		set_attr(&bundle, DEST_NODE, &val);
		val=23;
		set_attr(&bundle, DEST_SERV, &val);
		val=0x0002;
		set_attr(&bundle, SRC_NODE, &val);
		set_attr(&bundle, CUST_NODE, &val);
		val=42;
		set_attr(&bundle, SRC_SERV,&val);
		set_attr(&bundle, CUST_SERV, &val);

		val=0;
		set_attr(&bundle, FLAGS, &val);

		val=1;
		set_attr(&bundle, REP_NODE, &val);
		set_attr(&bundle, REP_SERV, &val);

		val = numbundles;
		set_attr(&bundle, TIME_STAMP_SEQ_NR, &val);
		val=1;
		set_attr(&bundle, LIFE_TIME, &val);
		val=4;
		set_attr(&bundle, TIME_STAMP, &val);

		/* Add the data block to the bundle */
		add_block(&bundle, 1, 2, databuf, DATASIZE);

		numbundles++;
		delete_bundle(&bundle);
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
