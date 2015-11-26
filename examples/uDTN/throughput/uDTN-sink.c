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

#include "FreeRTOS.h"
#include "task.h"

#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"

#include "net/uDTN/api.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/sdnv.h"
#include "net/uDTN/storage.h"
#include "dtn_process.h"

#ifdef CONF_BUNDLE_SIZE
#define BUNDLE_SIZE CONF_BUNDLE_SIZE
#else
#define BUNDLE_SIZE 64
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

#ifdef CONF_WAITING_TIME
#define WAITING_TIME CONF_WAITING_TIME
#endif


static void udtn_sink_process(void* p)
{
	static uint16_t bundles_recv = 0;
	static uint16_t bundles_error = 0;
	static TickType_t time_start = 0;
//	static uint8_t userdata[2];
//	static uint32_t old_seqno = 0xFFFFFFFF;
//	static struct mmem * bundle_outgoing;


#ifdef WAITING_TIME
	/* make this node wait a couple of minutes until startup */
	printf("Waiting %u seconds...\n", WAITING_TIME);
	watchdog_stop();
	unsigned long start = clock_seconds();
	while( (clock_seconds() - start) < WAITING_TIME );
	watchdog_start();
	printf("Starting Test\n");
#endif

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

	printf("Init done, starting test\n");

	while (1) {
		event_container_t ev;
		const bool event_received = dtn_process_wait_event(submit_data_to_application_event, pdMS_TO_TICKS(4 * 1000 * 1000), &ev);

		/* Check for timeout */
		if (!event_received) {
			printf("TEST_FAIL: Didn't receive enough bundles");
			break;
		}

//		leds_toggle(1);

		/* Verify the content of the bundle */
		struct bundle_block_t * block = bundle_get_payload_block(ev.bundlemem);
		int error = 0;

		if( block == NULL ) {
			printf("Payload: no block\n");
			error = 1;
		} else {
//			if( block->block_size != BUNDLE_SIZE ) {
//				printf("Payload: length is %d, should be %d\n", block->block_size, BUNDLE_SIZE);
//				error = 1;
//			}

//			for(i=0; i<BUNDLE_SIZE; i++) {
//				if( block->payload[i] != (i % 255) ) {
//					printf("Payload: byte %d mismatch. Should be %02X, is %02X\n", i, i, block->payload[i]);
//					error = 1;
//				}
//			}
		}

		if( error ) {
			bundles_error ++;

			/* Tell the agent, that we have processed the bundle */
			ev.event = dtn_processing_finished;
			agent_send_event(&ev);

			continue;
		}

//		static uint32_t seqno = 0;
//		bundle_get_attr(ev.bundlemem, TIME_STAMP_SEQ_NR, &seqno);
//		static uint32_t tmp = 0;
//		bundle_get_attr(bundle_incoming, SRC_NODE, &tmp);

//		if( seqno == old_seqno ) {
//			printf("Duplicate bundle, ignoring\n");

//			/* Tell the agent, that we have processed the bundle */
//			process_post(&agent_process, dtn_processing_finished, bundle_incoming);

//			continue;
//		}

//		old_seqno = seqno;

		/* Tell the agent, that we have processed the bundle */
		ev.event = dtn_processing_finished;
		agent_send_event(&ev);

		bundles_recv++;

		/* Start counting time after the first bundle arrived */
		if (bundles_recv == 1) {
			time_start = xTaskGetTickCount();
		}

		if (bundles_recv % REPORTING_INTERVAL == 0)
			printf("%u\n", bundles_recv);

		/* Report profiling data after receiving BUNDLES bundles
		   Ideally seq. no 0-999 */
		if (bundles_recv==BUNDLES) {
//			leds_off(1);
			const TickType_t time_stop = xTaskGetTickCount();
			const TickType_t diff = time_stop - time_start;

			/* use uint64_t, beacuse of throughput calulation */
			const uint32_t block_size = block->block_size;
			printf("size %lu\n", block_size);
			printf("time %lu ms\n", diff / portTICK_PERIOD_MS);

			const uint32_t bundles_per_sec = BUNDLES * 1000 * portTICK_PERIOD_MS / diff;
			printf("throughput %lu bundles/s\n", bundles_per_sec);
			printf("errors %u\n", bundles_error);

			const uint64_t bundles_payload = ((uint64_t)block_size) * BUNDLES * 1000 * portTICK_PERIOD_MS / diff;
			printf("throughput_bytes %lu bytes/s\n", (uint32_t)bundles_payload);

//			/* Packet loss in percent
//			   We received 1000 bundles, if seqno is 999 bundleloss is 0%
//			   If seqno is 1999 bundleloss is 50% (1000 received, 1000 lost) */
//			printf("packetloss %lu %%\n", (seqno-(BUNDLES-1))*100/(seqno+1));
		}

//		if( bundles_recv >= BUNDLES && BUNDLE_STORAGE.free_space(NULL) > 0 ) {
//			bundle_outgoing = bundle_create_bundle();

//			if( bundle_outgoing == NULL ) {
//				printf("create_bundle failed\n");
//				continue;
//			}

//			/* tmp already holds the src address of the sender */
//			bundle_set_attr(bundle_outgoing, DEST_NODE, &tmp);
//			tmp=25;
//			bundle_set_attr(bundle_outgoing, DEST_SERV, &tmp);

//			/* Bundle flags */
//			tmp=BUNDLE_FLAG_SINGLETON | BUNDLE_PRIORITY_EXPEDITED;
//			bundle_set_attr(bundle_outgoing, FLAGS, &tmp);

//			/* Bundle lifetime */
//			tmp=2000;
//			bundle_set_attr(bundle_outgoing, LIFE_TIME, &tmp);

//			/* Add the payload */
//			userdata[0] = 'o';
//			userdata[1] = 'k';
//			bundle_add_block(bundle_outgoing, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, userdata, 2);

//			/* Send out the bundle */
//			process_post(&agent_process, dtn_send_bundle_event, (void *) bundle_outgoing);

//			printf("bundle sent, waiting for event...\n");

//			/* Wait for the agent to process our outgoing bundle */
//			PROCESS_WAIT_UNTIL(ev == dtn_bundle_stored || ev == dtn_bundle_store_failed);

//			if( ev == dtn_bundle_stored ) {
//				/* Deactivate our registration, so that we do not receive bundles anymore */
//				reg.status = 0;
//				process_post(&agent_process, dtn_application_status_event, &reg);

//				watchdog_start();
//				PROCESS_EXIT();
//			} else {
//				printf("bundle send failed\n");
//			}
//		}
	}
}
/*---------------------------------------------------------------------------*/

bool init()
{
	QueueHandle_t event_queue = NULL;
	if ( !dtn_process_create_with_queue(udtn_sink_process, "Throughput", 0x100, 6, &event_queue) ) {
		return false;
	}

  return true;
}
