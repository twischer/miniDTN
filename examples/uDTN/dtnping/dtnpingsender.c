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

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "net/netstack.h"
#include "net/packetbuf.h"

#include "net/uDTN/api.h"
#include "dtn_process.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/sdnv.h"

#define DTN_PING_NODE		 2
#define DTN_PING_ENDPOINT	11
#define DTN_PING_LENGTH		64

#define DTN_SERVICE			(DTN_PING_ENDPOINT + 1)

/*---------------------------------------------------------------------------*/
static struct registration_api reg;
/*---------------------------------------------------------------------------*/
void dtnping_process(void* p)
{
	static uint32_t bundles_sent = 0;
	static TickType_t start;
	static TickType_t end;

	uint32_t tmp;

	static uint8_t payload_buffer[DTN_PING_LENGTH];
	uint32_t source_node;
	uint32_t source_service;
	uint32_t incoming_lifetime;

	struct mmem * bundlemem = NULL;
	struct bundle_block_t * block = NULL;

	/* Give agent time to initialize */
	vTaskDelay( pdMS_TO_TICKS(1000) );


	/* Register ping endpoint */
	reg.status = APP_ACTIVE;
	reg.event_queue = dtn_process_get_event_queue();
	reg.app_id = DTN_SERVICE;
	const event_container_t event = {
		.event = dtn_application_registration_event,
		.registration = &reg
	};
	agent_send_event(&event);

	printf("main: queue=%p app=%u\n",reg.event_queue, (unsigned int)reg.app_id);
	printf("ECHO ipn:%u.%u %u bytes of data.\n", DTN_PING_NODE, DTN_PING_ENDPOINT, DTN_PING_LENGTH);

	while (1) {
		vTaskDelay( pdMS_TO_TICKS(1000) );

		// Create the reply bundle
		bundlemem = bundle_create_bundle();
		if (!bundlemem) {
			printf("create_bundle failed\n");
			continue;
		}

		// Set the reply EID to the incoming bundle information
		tmp = DTN_PING_NODE;
		bundle_set_attr(bundlemem, DEST_NODE, &tmp);

		tmp = DTN_PING_ENDPOINT;
		bundle_set_attr(bundlemem, DEST_SERV, &tmp);

		// Make us the sender, the custodian and the report to
		tmp = dtn_node_id;
		bundle_set_attr(bundlemem, SRC_NODE, &tmp);
		bundle_set_attr(bundlemem, CUST_NODE, &tmp);
		bundle_set_attr(bundlemem, REP_NODE, &tmp);

		// Set our service to 11 [DTN_PING_ENDPOINT] (IBR-DTN expects that)
		tmp = DTN_SERVICE;
		bundle_set_attr(bundlemem, SRC_SERV, &tmp);
		bundle_set_attr(bundlemem, CUST_SERV, &tmp);
		bundle_set_attr(bundlemem, REP_SERV, &tmp);

		// Now set the flags
		tmp = BUNDLE_FLAG_SINGLETON;
		bundle_set_attr(bundlemem, FLAGS, &tmp);

		/**
		 * Hardcoded creation timestamp based on:
		 * date -j +%s   -    date -j 010100002000 +%s
		 */
		tmp = 0;
		bundle_set_attr(bundlemem, TIME_STAMP, &tmp);
		tmp = 0;
		bundle_set_attr(bundlemem, LIFE_TIME, &tmp);

		// Increment sequence number
		bundles_sent ++;

		// Put the sequence number into the first 4 bytes of the payload
		payload_buffer[0] = (bundles_sent & 0x000000FF) >>  0;
		payload_buffer[1] = (bundles_sent & 0x0000FF00) >>  8;
		payload_buffer[2] = (bundles_sent & 0x00FF0000) >> 16;
		payload_buffer[3] = (bundles_sent & 0xFF000000) >> 24;

		// Fill the rest of the payload
		int g;
		for(g=4; g<DTN_PING_LENGTH; g++) {
			payload_buffer[g] = g;
		}

		// Flag 0x08 is last_block Flag
		bundle_add_block(bundlemem, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload_buffer, DTN_PING_LENGTH);


		printf("Sending ping...\n");

		// And submit the bundle to the agent
		const event_container_t event = {
			.event = dtn_send_bundle_event,
			.bundlemem = bundlemem
		};
		agent_send_event(&event);

		start = xTaskGetTickCount();

		event_container_t ev;
		const bool event_received = dtn_process_wait_event(submit_data_to_application_event, pdMS_TO_TICKS(5000), &ev);
		end = xTaskGetTickCount();

		if(!event_received) {
			printf("timeout\n");
			continue;
		}

		// Reconstruct the bundle and mmem struct from the event
		bundlemem = ev.bundlemem;

		// Extract payload for further analysis
		block = bundle_get_payload_block(bundlemem);

		// Reconstruct sequence Number
		tmp = (((uint32_t) block->payload[3]) << 24) +
				(((uint32_t) block->payload[2]) << 16) +
				(((uint32_t) block->payload[1]) << 8) +
				((uint32_t) block->payload[0]);

		// Extract source information
		bundle_get_attr(bundlemem, SRC_NODE, &source_node);
		bundle_get_attr(bundlemem, SRC_SERV, &source_service);

		// Extract lifetime information
		bundle_get_attr(bundlemem, LIFE_TIME, &incoming_lifetime);

		// Tell the agent, that have processed the bundle
		const event_container_t finished_event = {
			.event = dtn_processing_finished,
			.bundlemem = bundlemem
		};
		agent_send_event(&finished_event);

		// Warn duplicate packets
		if( tmp != bundles_sent ) {
			printf("DUP %lu vs. %lu\n", tmp, bundles_sent);
		}

		// 64 bytes from ipn:2.11: seq=1 ttl=30 time=22.41 ms
		printf("%u bytes from ipn:%lu.%lu: seq=%lu ttl=%lu time=%lu ms\n",
				block->block_size,
				source_node,
				source_service,
				tmp,
				incoming_lifetime,
				(end - start) / portTICK_PERIOD_MS);
	}
}
/*---------------------------------------------------------------------------*/

bool init()
{
	if ( !dtn_process_create_other_stack(dtnping_process, "DTN Ping", 0x100) ) {
		return false;
	}

  return true;
}
