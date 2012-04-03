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

#include "contiki.h"
#include "watchdog.h"
#include "process.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "dev/leds.h"
#include "net/uDTN/API_registration.h"
#include "net/uDTN/API_events.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/sdnv.h"

#define DTN_PING_NODE		 2
#define DTN_PING_ENDPOINT	11
#define DTN_PING_LENGTH		64

/*---------------------------------------------------------------------------*/
PROCESS(dtnping_process, "DTN PING process");
AUTOSTART_PROCESSES(&dtnping_process);
static struct registration_api reg;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dtnping_process, ev, data)
{
	static struct etimer timer;
	static struct etimer timeout;
	static struct bundle_t bun;
	static uint32_t bundles_sent = 0;
	static clock_time_t start;
	clock_time_t end;

	uint32_t tmp;
	uint32_t payload_length;
	uint8_t offset;

	uint8_t payload_buffer[DTN_PING_LENGTH];
	uint32_t source_node;
	uint32_t source_service;
	uint32_t incoming_timestamp;
	uint32_t incoming_lifetime;

	PROCESS_BEGIN();

	agent_init();

	etimer_set(&timer,  CLOCK_SECOND*1);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	reg.status = 1;
	reg.application_process = &dtnping_process;
	reg.app_id = DTN_PING_ENDPOINT;
	process_post(&agent_process, dtn_application_registration_event, &reg);

	etimer_set(&timer, CLOCK_SECOND);

	printf("ECHO ipn:%u.%u %u bytes of data.\n", DTN_PING_NODE, DTN_PING_ENDPOINT, DTN_PING_LENGTH);

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		// Create the reply bundle
		create_bundle(&bun);

		// Set the reply EID to the incoming bundle information
		tmp = DTN_PING_NODE;
		set_attr(&bun, DEST_NODE, &tmp);

		tmp = DTN_PING_ENDPOINT;
		set_attr(&bun, DEST_SERV, &tmp);

		// Make us the sender, the custodian and the report to
		tmp = dtn_node_id;
		set_attr(&bun, SRC_NODE, &tmp);
		set_attr(&bun, CUST_NODE, &tmp);
		set_attr(&bun, CUST_SERV, &tmp);
		set_attr(&bun, REP_NODE, &tmp);
		set_attr(&bun, REP_SERV, &tmp);

		// Set our service to 11 [DTN_PING_ENDPOINT] (IBR-DTN expects that)
		tmp = DTN_PING_ENDPOINT;
		set_attr(&bun, SRC_SERV, &tmp);

		// Now set the flags
		tmp = 0x10; // Endpoint is Singleton
		set_attr(&bun, FLAGS, &tmp);

		// Set the sequence number to the number of bundles sent
		set_attr(&bun, TIME_STAMP_SEQ_NR, &bundles_sent);

		// Set the same lifetime and timestamp as the incoming bundle
		/**
		 * Hardcoded creation timestamp based on:
		 * date -j +%s   -    date -j 010100002000 +%s
		 */
		tmp = 0;
		set_attr(&bun, TIME_STAMP, &tmp);
		tmp = 0;
		set_attr(&bun, LIFE_TIME, &tmp);

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
		add_block(&bun, 1, 0x08, payload_buffer, DTN_PING_LENGTH);

		// And submit the bundle to the agent
		process_post(&agent_process, dtn_send_bundle_event, (void *) &bun);
		start = clock_time();

		etimer_set(&timeout, CLOCK_SECOND * 5);

		PROCESS_WAIT_EVENT_UNTIL(ev == submit_data_to_application_event || etimer_expired(&timeout));
		end = clock_time();

		if( etimer_expired(&timeout) ) {
			printf("timeout\n");
			etimer_set(&timer, CLOCK_SECOND);
			continue;
		}

		// Reconstruct the bundle struct from the event
		struct bundle_t * bundle;
		bundle = (struct bundle_t *) data;

		leds_toggle(1);

		// Extract payload for further analysis
		// Processing flags
		offset = sdnv_decode(bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + 1, 4, &tmp) + 1;

		// Payload Length
		offset += sdnv_decode(bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, 4, &payload_length);
		memcpy(payload_buffer, bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, payload_length);

		// Extract source information
		sdnv_decode(bundle->mem.ptr+bundle->offset_tab[SRC_NODE][OFFSET],
				bundle->offset_tab[SRC_NODE][STATE], &source_node);
		sdnv_decode(bundle->mem.ptr+bundle->offset_tab[SRC_SERV][OFFSET],
				bundle->offset_tab[SRC_SERV][STATE], &source_service);

		// Extract lifetime information
		sdnv_decode(bundle->mem.ptr+bundle->offset_tab[LIFE_TIME][OFFSET],
				bundle->offset_tab[LIFE_TIME][STATE], &incoming_lifetime);

		// Delete the incoming bundle
		delete_bundle(bundle);

		// Reconstruct sequence Number
		tmp = (((uint32_t) payload_buffer[3]) << 24) +
				(((uint32_t) payload_buffer[2]) << 16) +
				(((uint32_t) payload_buffer[1]) << 8) +
				((uint32_t) payload_buffer[0]);

		// Warn duplicate packets
		if( tmp != bundles_sent ) {
			printf("DUP %lu vs. %lu\n", tmp, bundles_sent);
		}

		// 64 bytes from ipn:2.11: seq=1 ttl=30 time=22.41 ms
		printf("%lu bytes from ipn:%lu.%lu: seq=%lu ttl=%lu time=%d ms\n",
				payload_length,
				source_node,
				source_service,
				tmp,
				incoming_lifetime,
				((end - start) * 1000) / CLOCK_SECOND);

		// ping again in 1 second
		etimer_set(&timer, CLOCK_SECOND);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
