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
#include "sys/profiling.h"
#include "sys/test.h"
#include "process.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "etimer.h"

#include "net/uDTN/api.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/sdnv.h"

#ifdef CONF_BUNDLES
#define BUNDLES CONF_BUNDLES
#else
#define BUNDLES 1000
#endif

/*---------------------------------------------------------------------------*/
PROCESS(udtn_sink_process, "uDTN Sink process");
AUTOSTART_PROCESSES(&udtn_sink_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udtn_sink_process, ev, data)
{
	static struct registration_api reg;
	static struct etimer timer;
	static uint16_t bundles_recv = 0;
	static uint16_t bundles_error = 0;
	static uint32_t time_start, time_stop;
	static uint8_t userdata[2];
	uint32_t tmp;
	static uint32_t seqno;
	struct mmem * bundle_incoming;
	static struct mmem * bundle_outgoing;

	PROCESS_BEGIN();
	profiling_init();
	profiling_start();

	/* Wait for the agent to be initialized */
	PROCESS_PAUSE();

	/* Register our endpoint to receive bundles */
	reg.status = APP_ACTIVE;
	reg.application_process = PROCESS_CURRENT();
	reg.app_id = 25;
	process_post(&agent_process, dtn_application_registration_event, &reg);

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
			if (clock_seconds()-(time_start/CLOCK_SECOND) > 18000) {
				profiling_stop();
				watchdog_stop();
				profiling_report("timeout", 0);
				watchdog_start();
				TEST_FAIL("Didn't receive enough bundles");
				PROCESS_EXIT();
			}

			if( ev != submit_data_to_application_event ) {
				/* No timeout - restart while-loop */
				continue;
			}
		}

		/* If the etimer didn't expire we're getting a submit_data_to_application_event */
		bundle_incoming = (struct mmem *) data;

		leds_toggle(1);

		/* Verify the content of the bundle */
		struct bundle_block_t * block = bundle_get_payload_block(bundle_incoming);
		int i;
		int error = 0;

		if( block == NULL ) {
			printf("Payload: no block\n");
			error = 1;
		} else {
			if( block->block_size != 80 ) {
				printf("Payload: length is %d, should be 80\n", block->block_size);
				error = 1;
			}

			for(i=0; i<80; i++) {
				if( block->payload[i] != i ) {
					printf("Payload: byte %d mismatch. Should be %02X, is %02X\n", i, i, block->payload[i]);
					error = 1;
				}
			}
		}

		if( error ) {
			bundles_error ++;

			/* Tell the agent, that we have processed the bundle */
			process_post(&agent_process, dtn_processing_finished, bundle_incoming);

			continue;
		}

		bundle_get_attr(bundle_incoming, TIME_STAMP_SEQ_NR, &seqno);
		bundle_get_attr(bundle_incoming, SRC_NODE, &tmp);

		/* Tell the agent, that we have processed the bundle */
		process_post(&agent_process, dtn_processing_finished, bundle_incoming);

		bundles_recv++;

		/* Start counting time after the first bundle arrived */
		if (bundles_recv == 1) {
			time_start = test_precise_timestamp(NULL);
		}

		if (bundles_recv%50 == 0)
			printf("%u\n", bundles_recv);

		/* Report profiling data after receiving BUNDLES bundles
		   Ideally seq. no 0-999 */
		if (bundles_recv==BUNDLES) {
			leds_off(1);
			profiling_stop();
			time_stop = test_precise_timestamp(NULL);

			bundle_outgoing = bundle_create_bundle();

			if( bundle_outgoing == NULL ) {
				printf("create_bundle failed\n");
				continue;
			}

			/* tmp already holds the src address of the sender */
			bundle_set_attr(bundle_outgoing, DEST_NODE, &tmp);
			tmp=25;
			bundle_set_attr(bundle_outgoing, DEST_SERV, &tmp);

			/* Bundle flags */
			tmp=BUNDLE_FLAG_SINGLETON;
			bundle_set_attr(bundle_outgoing, FLAGS, &tmp);

			/* Bundle lifetime */
			tmp=2000;
			bundle_set_attr(bundle_outgoing, LIFE_TIME, &tmp);

			/* Add the payload */
			userdata[0] = 'o';
			userdata[1] = 'k';
			bundle_add_block(bundle_outgoing, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, userdata, 2);

			/* Send out the bundle */
			process_post(&agent_process, dtn_send_bundle_event, (void *) bundle_outgoing);

			/* Wait for the agent to process our outgoing bundle */
			PROCESS_WAIT_UNTIL(ev == dtn_bundle_stored);

			/* Deactivate our registration, so that we do not receive bundles anymore */
			reg.status = 0;
			process_post(&agent_process, dtn_application_status_event, &reg);

			watchdog_stop();
			profiling_report("recv-bundles", 0);
			TEST_REPORT("throughput", BUNDLES*CLOCK_SECOND, time_stop-time_start, "bundles/s");
			TEST_REPORT("errors", bundles_error, 1, "erronous bundles");

			/* Packet loss in percent
			   We received 1000 bundles, if seqno is 999 bundleloss is 0%
			   If seqno is 1999 bundleloss is 50% (1000 received, 1000 lost) */
			TEST_REPORT("packetloss", (seqno-(BUNDLES-1))*100, seqno+1, "\%");
			TEST_PASS();
			watchdog_start();
			PROCESS_EXIT();
		}
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
