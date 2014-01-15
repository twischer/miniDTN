/*
 * Copyright (c) 2014, Wolf-Bastian Pšttner.
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
 */

#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "process.h"

#include "dtn_apps.h"
#include "net/uDTN/api.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/bundle.h"

#define DTN_PING_ENDPOINT	11

/*---------------------------------------------------------------------------*/
PROCESS(dtnpingecho_process, "DTN Ping Echo process");
static struct registration_api reg;
/*---------------------------------------------------------------------------*/
int dtnpingecho_init(void) {
	process_start(&dtnpingecho_process, NULL);
	return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dtnpingecho_process, ev, data)
{
	static uint32_t bundles_recv = 0;
	uint32_t tmp;

	uint32_t source_node;
	uint32_t source_service;
	uint32_t incoming_lifetime;

	struct mmem * bundlemem = NULL;
	struct bundle_block_t * block = NULL;

	// preserve the payload to send it back
	uint8_t payload_buffer[64];
	uint8_t payload_length;

	PROCESS_BEGIN();

	/* Give agent time to initialize */
	PROCESS_PAUSE();

	/* Register ping endpoint */
	reg.status = APP_ACTIVE;
	reg.application_process = PROCESS_CURRENT();;
	reg.app_id = DTN_PING_ENDPOINT;
	process_post(&agent_process, dtn_application_registration_event,&reg);

	printf("DTN Ping Echo running on ipn:%lu.%lu\n", dtn_node_id, reg.app_id);

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == submit_data_to_application_event);

		// Reconstruct the bundle struct from the event
		bundlemem = (struct mmem *) data;

		block = bundle_get_payload_block(bundlemem);
		if( block == NULL ) {
			printf("No payload block\n");
			continue;
		}

		if (block->block_size > 64) {
			printf("Payload too big, clamping to maximum size.\n");
			payload_length = 64;
		} else {
			payload_length = block->block_size;
		}
		memcpy(payload_buffer, block->payload, payload_length);

		// Extract the source information to send a reply back
		bundle_get_attr(bundlemem, SRC_NODE, &source_node);
		bundle_get_attr(bundlemem, SRC_SERV, &source_service);

		// Extract lifetime from incoming bundle
		bundle_get_attr(bundlemem, LIFE_TIME, &incoming_lifetime);

		bundles_recv++;
		printf("PING %lu of %u bytes received from ipn:%lu.%lu\n", bundles_recv, block->block_size, source_node, source_service);

		// Tell the agent, that have processed the bundle
		process_post(&agent_process, dtn_processing_finished, bundlemem);
		bundlemem = NULL;
		block = NULL;

		// Create the reply bundle
		bundlemem = bundle_create_bundle();
		if (!bundlemem) {
			printf("create_bundle failed\n");
			continue;
		}

		// Set the reply EID to the incoming bundle information
		bundle_set_attr(bundlemem, DEST_NODE, &source_node);
		bundle_set_attr(bundlemem, DEST_SERV, &source_service);

		// Set our service to 11 [DTN_PING_ENDPOINT] (IBR-DTN expects that)
		tmp = DTN_PING_ENDPOINT;
		bundle_set_attr(bundlemem, SRC_SERV, &tmp);

		// Now set the flags
		tmp = BUNDLE_FLAG_SINGLETON;
		bundle_set_attr(bundlemem, FLAGS, &tmp);

		// Set the same lifetime and timestamp as the incoming bundle
		bundle_set_attr(bundlemem, LIFE_TIME, &incoming_lifetime);

		// Copy payload from incoming bundle
		bundle_add_block(bundlemem, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload_buffer, payload_length);

		// And submit the bundle to the agent
		process_post(&agent_process, dtn_send_bundle_event, (void *) bundlemem);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
const struct dtn_app dtnpingecho = {
		.name = "DTN Ping Echo",
		.init = dtnpingecho_init,
};
/*---------------------------------------------------------------------------*/
