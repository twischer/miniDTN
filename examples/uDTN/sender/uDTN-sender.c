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
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "mmem.h"
#include "sys/test.h"
#include "sys/profiling.h"
#include "watchdog.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/api.h"
#include "net/uDTN/storage.h"
#include "net/uDTN/discovery.h"

#ifndef CONF_SEND_TO_NODE
#error "I need a destination node - set CONF_SEND_TO_NODE"
#endif

#ifdef CONF_BUNDLES
#define BUNDLES CONF_BUNDLES
#else
#define BUNDLES 1000
#endif

#ifdef CONF_REPORT
#define REPORT 1
#endif

/*---------------------------------------------------------------------------*/
PROCESS(udtn_sender_process, "uDTN Sender process");
AUTOSTART_PROCESSES(&udtn_sender_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udtn_sender_process, ev, data)
{
	uint8_t i;
	int n;
	static struct etimer timer;
	static struct registration_api reg;
	static uint16_t bundles_sent = 0;
	static uint32_t time_start, time_stop;
	uint8_t userdata[80];
	uint32_t tmp;
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
	process_post(&agent_process, dtn_application_registration_event,&reg);

	/* Profile initialization separately */
	profiling_stop();
	watchdog_stop();
	profiling_report("init", 0);
	watchdog_start();

	/* Wait until a neighbour has been discovered */
	printf("Waiting for neighbour to appear...\n");
	while( DISCOVERY.neighbours() == NULL ) {
		PROCESS_PAUSE();
	}

	/* Give the receiver a second to start up */
	etimer_set(&timer, CLOCK_SECOND);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));

	printf("Init done, starting test\n");

	profiling_init();
	profiling_start();

	/* Send ourselves the initial event */
	process_post(&udtn_sender_process, PROCESS_EVENT_CONTINUE, NULL);

	/* Note down the starting time */
	time_start = test_precise_timestamp(NULL);

	while(1) {
		/* Wait for the next incoming event */
		PROCESS_WAIT_EVENT();

		/* Check for timeout */
		if (clock_seconds()-(time_start/CLOCK_SECOND) > 18000) {
			profiling_stop();
			watchdog_stop();
			profiling_report("timeout", 0);
			watchdog_start();

			TEST_FAIL("Didn't receive ack from sink");

			PROCESS_EXIT();
		}

		/* We received a bundle - assume it is the sink telling us to
		 * stop sending */
		if (ev == submit_data_to_application_event) {
			struct mmem *recv = NULL;
			recv = (struct mmem *) data;

			/* Tell the agent, that we have processed the bundle */
			process_post(&agent_process, dtn_processing_finished, recv);

			profiling_stop();
			watchdog_stop();
			profiling_report("send-bundles", 0);
			watchdog_start();

			TEST_REPORT("throughput", BUNDLES*CLOCK_SECOND, time_stop-time_start, "bundles/s");
			TEST_PASS();

			PROCESS_EXIT();
		}

		/* Wait for the agent to process our outgoing bundle */
		if( ev != dtn_bundle_stored && ev != dtn_bundle_store_failed && ev != PROCESS_EVENT_CONTINUE ) {
			continue;
		}

		if( ev == dtn_bundle_store_failed ) {
			/* Sending the last bundle failed, do not send this time round */
			bundles_sent--;
			process_post(&udtn_sender_process, PROCESS_EVENT_CONTINUE, NULL);
			continue;
		}

		/* Only proceed, when we have enough storage left */
		if( BUNDLE_STORAGE.free_space(NULL) < 1 ) {
			process_post(&udtn_sender_process, PROCESS_EVENT_CONTINUE, NULL);
			continue;
		}

		/* Stop profiling if we've sent BUNDLES bundles. We still need to send
		 * more since some might have been lost on the way */
		if (bundles_sent == BUNDLES) {
			profiling_stop();
			/* Note down the time of the last bundle */
			time_stop = test_precise_timestamp(NULL);
		}

		/* Allocate memory for the outgoing bundle */
		bundle_outgoing = bundle_create_bundle();

		if( bundle_outgoing == NULL ) {
			printf("create_bundle failed\n");
			continue;
		}

		/* Source, destination, custody and report-to nodes and services*/
		tmp=CONF_SEND_TO_NODE;
		bundle_set_attr(bundle_outgoing, DEST_NODE, &tmp);
		tmp=25;
		bundle_set_attr(bundle_outgoing, DEST_SERV, &tmp);

		/* Bundle flags */
		tmp=BUNDLE_FLAG_SINGLETON;
#if REPORT
		/* Enable bundle delivery report */
		tmp |= BUNDLE_FLAG_REP_DELIV;
#endif
		bundle_set_attr(bundle_outgoing, FLAGS, &tmp);

		/* Bundle lifetime */
		tmp=2000;
		bundle_set_attr(bundle_outgoing, LIFE_TIME, &tmp);

		/* Add the payload */
		for(i=0; i<80; i++)
			userdata[i] = i;
		n = bundle_add_block(bundle_outgoing, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, userdata, 80);
		if( !n ) {
			printf("not enough room for block\n");
			bundle_decrement(bundle_outgoing);
			continue;
		}

		/* Hand the bundle over to the agent */
		process_post(&agent_process, dtn_send_bundle_event, (void *) bundle_outgoing);

		bundles_sent++;
		/* Show progress every 50 bundles */
		if (bundles_sent%50 == 0)
			printf("%i\n", bundles_sent);
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
