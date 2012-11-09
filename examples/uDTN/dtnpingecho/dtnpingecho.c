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

#include "net/uDTN/api.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/sdnv.h"

#define DTN_PING_ENDPOINT	11

/*---------------------------------------------------------------------------*/
PROCESS(dtnping_process, "DTN PING process");
AUTOSTART_PROCESSES(&dtnping_process);
static struct registration_api reg;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dtnping_process, ev, data)
{
	static struct etimer timer;
	static uint32_t bundles_recv = 0;
	uint32_t tmp;

	uint32_t source_node;
	uint32_t source_service;
	uint32_t incoming_timestamp;
	uint32_t incoming_lifetime;

	struct mmem * bundlemem = NULL;
	struct bundle_t * bundle = NULL;
	struct bundle_block_t * block = NULL;

	PROCESS_BEGIN();

	agent_init();

	etimer_set(&timer, CLOCK_SECOND*1);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	reg.status = 1;
	reg.application_process = &dtnping_process;
	reg.app_id = DTN_PING_ENDPOINT;
	process_post(&agent_process, dtn_application_registration_event,&reg);

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == submit_data_to_application_event);

		// Reconstruct the bundle struct from the event
		bundlemem = (struct mmem *) data;
		bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

		// preserve the payload to send it back
		uint8_t payload_buffer[64];
		uint8_t payload_length;

		block = get_payload_block(bundlemem);
		payload_length = block->block_size;
		if (payload_length > 64) {
			printf("Payload too big, clamping to maximum size.\n");
		}
		memcpy(payload_buffer, block->payload, payload_length);

		// Extract the source information to send a reply back
		get_attr(bundlemem, SRC_NODE, &source_node);
		get_attr(bundlemem, SRC_SERV, &source_service);

		// Extract timestamp and lifetime from incoming bundle
		get_attr(bundlemem, TIME_STAMP, &incoming_timestamp);
		get_attr(bundlemem, LIFE_TIME, &incoming_lifetime);

		// Tell the agent, that have processed the bundle
		process_post(&agent_process, dtn_processing_finished, bundlemem);

		bundles_recv++;
		printf("PING %lu received\n", bundles_recv);

		// Create the reply bundle
		bundlemem = create_bundle();
		if (!bundlemem) {
			printf("create_bundle failed\n");
			continue;
		}

		// Set the reply EID to the incoming bundle information
		set_attr(bundlemem, DEST_NODE, &source_node);
		set_attr(bundlemem, DEST_SERV, &source_service);

		// Make us the sender, the custodian and the report to
		tmp = dtn_node_id;
		set_attr(bundlemem, SRC_NODE, &tmp);
		set_attr(bundlemem, CUST_NODE, &tmp);
		set_attr(bundlemem, CUST_SERV, &tmp);
		set_attr(bundlemem, REP_NODE, &tmp);
		set_attr(bundlemem, REP_SERV, &tmp);

		// Set our service to 11 [DTN_PING_ENDPOINT] (IBR-DTN expects that)
		tmp = DTN_PING_ENDPOINT;
		set_attr(bundlemem, SRC_SERV, &tmp);

		// Now set the flags
		tmp = BUNDLE_FLAG_SINGLETON;
		set_attr(bundlemem, FLAGS, &tmp);

		// Set the same lifetime and timestamp as the incoming bundle
		set_attr(bundlemem, LIFE_TIME, &incoming_lifetime);
		set_attr(bundlemem, TIME_STAMP, &incoming_timestamp);

		// Copy payload from incoming bundle
		add_block(bundlemem, 1, 0, payload_buffer, payload_length);

		// And submit the bundle to the agent
		process_post(&agent_process, dtn_send_bundle_event, (void *) bundlemem);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
