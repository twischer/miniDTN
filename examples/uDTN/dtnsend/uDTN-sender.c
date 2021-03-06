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

#include "FreeRTOS.h"
#include "task.h"

#include "net/netstack.h"
#include "net/packetbuf.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/api.h"
#include "net/uDTN/storage.h"
#include "net/uDTN/discovery.h"
#include "dtn_process.h"

#ifndef CONF_SEND_TO_NODE
#error "I need a destination node - set CONF_SEND_TO_NODE"
#endif

#define SEND_DELAY		0

#ifdef CONF_BUNDLE_SIZE
#define BUNDLE_SIZE CONF_BUNDLE_SIZE
#else
#define BUNDLE_SIZE 80
#endif

#ifdef CONF_BUNDLES
#define BUNDLES CONF_BUNDLES
#else
#define BUNDLES 1000
#endif

#ifdef CONF_REPORTING_INTERVAL
#define REPORTING_INTERVAL CONF_REPORTING_INTERVAL
#else
#define REPORTING_INTERVAL 50
#endif

#ifdef CONF_REPORT
#define REPORT 1
#endif

#ifdef CONF_WAITING_TIME
#define WAITING_TIME CONF_WAITING_TIME
#endif

/*---------------------------------------------------------------------------*/
static void udtn_sender_process(void* p)
{
	static uint16_t bundles_sent = 0;
//	static uint32_t time_start, time_stop;


	/* Wait for the agent to be initialized */
	vTaskDelay( pdMS_TO_TICKS(1000) );


	/* Register our endpoint to receive bundles */
	static struct registration_api reg;
	reg.status = APP_ACTIVE;
	reg.event_queue = dtn_process_get_event_queue();
	reg.app_id = 25;
	const event_container_t event = {
		.event = dtn_application_registration_event,
		.registration = &reg
	};
	agent_send_event(&event);


#ifndef WAITING_TIME
	printf("Waiting for neighbour ipn:%lu to appear...\n", (uint32_t)CONF_SEND_TO_NODE);
	while( !DISCOVERY.is_neighbour(CONF_SEND_TO_NODE) ) {
		vTaskDelay( pdMS_TO_TICKS(1000) );
	}
#endif

	/* Give the receiver a second to start up */
	vTaskDelay( pdMS_TO_TICKS(6000) );


	/* Add the payload */
	static uint8_t userdata[BUNDLE_SIZE];
	for(int i=0; i<BUNDLE_SIZE; i++)
		userdata[i] = (i & 0xFF);

	printf("Init done, starting test\n");

	/* Note down the starting time */
//	time_start = test_precise_timestamp();

	while(1) {
//		/* Check for timeout */
//		if (clock_seconds()-(time_start/CLOCK_SECOND) > 18000) {
//			profiling_stop();
//			watchdog_stop();
//			profiling_report("timeout", 0);
//			watchdog_start();

//			TEST_FAIL("Didn't receive ack from sink");

//			PROCESS_EXIT();
//		}

//		/* We received a bundle - assume it is the sink telling us to
//		 * stop sending */
//		if (ev == submit_data_to_application_event) {
//			struct mmem *recv = NULL;
//			recv = (struct mmem *) data;

//			/* We can read several attributes as defined in bundle.h */
//			uint32_t source_node;
//			bundle_get_attr(recv, SRC_NODE, &source_node);

//			/* We can obtain the bundle payload block like so: */
//			struct bundle_block_t * block = bundle_get_payload_block(recv);

//			/* Tell the agent, that we have processed the bundle */
//			process_post(&agent_process, dtn_processing_finished, recv);

//			if( source_node != CONF_SEND_TO_NODE || block->payload[0] != 'o' || block->payload[1] != 'k' ) {
//				continue;
//			}

//			profiling_stop();
//			watchdog_stop();
//			profiling_report("send-bundles", 0);
//			watchdog_start();

//			TEST_REPORT("throughput", BUNDLES*CLOCK_SECOND, time_stop-time_start, "bundles/s");
//			TEST_REPORT("throughput_bytes", BUNDLES * BUNDLE_SIZE * CLOCK_SECOND, time_stop-time_start, "bytes/s");
//			TEST_PASS();

//			PROCESS_EXIT();
//		}


		/* Only proceed, when we have enough storage left */
		if(BUNDLE_STORAGE.free_space(NULL) <= 2) {
			BUNDLE_STORAGE.wait_for_changes();
			continue;
		}

//		/* Stop profiling if we've sent BUNDLES bundles. We still need to send
//		 * more since some might have been lost on the way */
//		if (bundles_sent == BUNDLES) {
//			printf("Stopping profiling and taking time...\n");
//			profiling_stop();
//			/* Note down the time of the last bundle */
//			time_stop = test_precise_timestamp();
//			printf("\tdone\n");
//		}

		/* Allocate memory for the outgoing bundle */
		struct mmem* bundle_outgoing = bundle_create_bundle();

		if( bundle_outgoing == NULL ) {
			printf("create_bundle failed\n");
			continue;
		}
		configASSERT( ((struct bundle_t *)MMEM_PTR(bundle_outgoing))->source_event_queue != NULL );

		/* Source, destination, custody and report-to nodes and services*/
		uint32_t tmp=CONF_SEND_TO_NODE;
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

		if (bundle_add_block(bundle_outgoing, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, userdata, BUNDLE_SIZE) < 0) {
			printf("not enough room for block\n");
			bundle_decrement(bundle_outgoing);
			continue;
		}

		/* Hand the bundle over to the agent */
		const event_container_t send_event = {
			.event = dtn_send_bundle_event,
			.bundlemem = bundle_outgoing
		};
		configASSERT( ((struct bundle_t *)send_event.bundlemem->ptr)->source_event_queue );
		agent_send_event(&send_event);


		while (true) {
			event_container_t ev;
			const bool event_received = dtn_process_wait_any_event(portMAX_DELAY, &ev);
			if (!event_received) {
				printf("Timeout\n");
				break;
			}

			if (ev.event == dtn_bundle_store_failed) {
				printf("Send failed\n");
				taskYIELD();
				break;
			}

			if (ev.event == dtn_bundle_stored) {
				bundles_sent++;

#if (SEND_DELAY <= 0)
				/* Show progress every REPORTING_INTERVAL bundles */
				if (bundles_sent % REPORTING_INTERVAL == 0) {
					printf("%i\n", bundles_sent);
				}
#else
				printf("%i\n", bundles_sent);
				vTaskDelay(pdMS_TO_TICKS(SEND_DELAY));
#endif
				break;
			}

			/* if there was an unknown event, wait for the next one */
		}
	}
}
/*---------------------------------------------------------------------------*/

bool init()
{
	if ( !dtn_process_create_other_stack(udtn_sender_process, "DTN Sender", 0x200) ) {
		return false;
	}

  return true;
}
