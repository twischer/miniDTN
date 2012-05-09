/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * $Id: hello-world.c,v 1.1 2006/10/02 21:46:46 adamdunkels Exp $
 */

/**
 * \file
 *	   A very simple Contiki application showing how Contiki programs look
 * \author
 *	   Adam Dunkels <adam@sics.se>
 */

#include <stdio.h>

#include "contiki.h"
#include "watchdog.h"

//#include "../platform/avr-raven/cfs-coffee-arch.h"
//#include "cfs.h"

#include "sys/profiling.h"
#include "sys/test.h"

#include "process.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/uDTN/API_registration.h"
#include "net/uDTN/API_events.h"
#include "net/uDTN/agent.h"
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/sdnv.h"
#include "etimer.h"


/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
static struct registration_api reg;
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(hello_world_process, ev, data)
{
	static struct etimer timer;
	static struct bundle_t bun;
	static uint16_t bundles_recv = 0;
	static uint32_t time_start, time_stop;
	static uint8_t userdata[2];
	uint32_t tmp;
	uint32_t seqno;
        clock_time_t now;
        unsigned short now_fine;

	PROCESS_BEGIN();
	profiling_init();
	profiling_start();

	agent_init();
	reg.status=1;
	reg.application_process=&hello_world_process;
	reg.app_id=25;
	printf("MAIN: event= %u\n",dtn_application_registration_event);
	printf("main app_id %lu process %p\n", reg.app_id, &agent_process);
	etimer_set(&timer,  CLOCK_SECOND*5);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	process_post(&agent_process, dtn_application_registration_event,&reg);

	/* Profile initialization separately */
	profiling_stop();
	watchdog_stop();
	profiling_report("init", 0);
	watchdog_start();
	printf("Init done, starting test\n");

	profiling_init();
	profiling_start();

	while (1) {
		etimer_set(&timer, CLOCK_SECOND*10);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer) ||
				ev == submit_data_to_application_event);

		/* Check for timeout */
		if (etimer_expired(&timer)) {
			if (clock_seconds()-(time_start/CLOCK_SECOND) > 900) {
				profiling_stop();
				watchdog_stop();
				profiling_report("timeout", 0);
				watchdog_start();
				TEST_FAIL("Didn't receive enough bundles");
				PROCESS_EXIT();
			}
			/* No timeout - restart while-loop */
			continue;
		}

		/* If the etimer didn't expire we're getting a submit_data_to_application_event */
		struct bundle_t *bundle;
		bundle = (struct bundle_t *) data;

		leds_toggle(1);

		get_attr(bundle, TIME_STAMP_SEQ_NR, &seqno);
		get_attr(bundle, SRC_NODE, &tmp);
		delete_bundle(bundle);

		bundles_recv++;
		/* Start counting time after the first bundle arrived */
		if (bundles_recv == 1) {
		        do {
		                now_fine = clock_time();
		                now = clock_seconds();
		        } while (now_fine != clock_time());
		        time_start = ((unsigned long)now)*CLOCK_SECOND + now_fine%CLOCK_SECOND;
		}

		if (bundles_recv%50 == 0)
			printf("%u\n", bundles_recv);

		/* Report profiling data after receiving 1000 bundles
		   Ideally seq. no 0-999 */
		if (bundles_recv==1000) {
			leds_off(1);
			profiling_stop();
		        do {
		                now_fine = clock_time();
		                now = clock_seconds();
		        } while (now_fine != clock_time());
		        time_stop = ((unsigned long)now)*CLOCK_SECOND + now_fine%CLOCK_SECOND;

			create_bundle(&bun);

			/* tmp already holdy the src address of the sender */
			set_attr(&bun, DEST_NODE, &tmp);
			tmp=25;
			set_attr(&bun, DEST_SERV, &tmp);
			tmp=dtn_node_id;
			set_attr(&bun, SRC_NODE, &tmp);
			set_attr(&bun, SRC_SERV,&tmp);
			set_attr(&bun, CUST_NODE, &tmp);
			set_attr(&bun, CUST_SERV, &tmp);

			tmp=0;
			set_attr(&bun, FLAGS, &tmp);
			tmp=1;
			set_attr(&bun, REP_NODE, &tmp);
			set_attr(&bun, REP_SERV, &tmp);

			/* Set the sequence number to the number of budles sent */
			tmp = 1;
			set_attr(&bun, TIME_STAMP_SEQ_NR, &tmp);

			tmp=2000;
			set_attr(&bun, LIFE_TIME, &tmp);
			tmp=4;
			set_attr(&bun, TIME_STAMP, &tmp);

			/* Add the payload */
			userdata[0] = 'o';
			userdata[1] = 'k';
			add_block(&bun, 1, 2, userdata, 2);

			process_post(&agent_process, dtn_send_bundle_event, (void *) &bun);

			watchdog_stop();
			profiling_report("recv-1000", 0);
			TEST_REPORT("throughput", 1000L*CLOCK_SECOND, time_stop-time_start, "bundles/s");
			/* Packet loss in percent
			   We received 1000 bundles, if seqno is 999 bundleloss is 0%
			   If seqno is 1999 bundleloss is 50% (1000 received, 1000 lost) */
			TEST_REPORT("packetloss", (seqno-999)*100, seqno+1, "\%");
			TEST_PASS();
			watchdog_start();
			PROCESS_EXIT();
		}
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
