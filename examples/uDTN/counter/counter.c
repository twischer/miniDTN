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

#define DEST_NODE_EID    10
#define DEST_ENDPOINT    26

#define DTN_SERVICE      (DEST_ENDPOINT + 1)


void counter_process(void* p)
{
	/* Give agent time to initialize */
	vTaskDelay( pdMS_TO_TICKS(1000) );

	/* Register counting endpoint */
	static struct registration_api reg;
	reg.status = APP_ACTIVE;
	reg.event_queue = dtn_process_get_event_queue();
	reg.app_id = DTN_SERVICE;
	const event_container_t reg_event = {
		.event = dtn_application_registration_event,
		.registration = &reg
	};
	agent_send_event(&reg_event);

	printf("main: queue=%p app=%u\n",reg.event_queue, (unsigned int)reg.app_id);
	printf("Counter sending to ipn:%u.%u\n", DEST_NODE_EID, DEST_ENDPOINT);

	while (1) {
		vTaskDelay( pdMS_TO_TICKS(1000) );

		struct mmem* const bundlemem = bundle_create_bundle();
		if (bundlemem == NULL) {
			printf("create_bundle failed\n");
			continue;
		}

		// Set the reply EID to the incoming bundle information
		const uint32_t dest_node = DEST_NODE_EID;
		bundle_set_attr(bundlemem, DEST_NODE, &dest_node);

		const uint32_t dest_service = DEST_ENDPOINT;
		bundle_set_attr(bundlemem, DEST_SERV, &dest_service);

//		const uint32_t src_node = dtn_node_id;
//		bundle_set_attr(bundlemem, SRC_NODE, &src_node);

//		const uint32_t src_service = DTN_SERVICE;
//		bundle_set_attr(bundlemem, SRC_SERV, &src_service);

		// Now set the flags
		// TODO wirklich benötigt
		const uint32_t tmp = BUNDLE_FLAG_SINGLETON;
		bundle_set_attr(bundlemem, FLAGS, &tmp);


		static uint32_t counter = 0;
		/* One byte represents max 3 digits.
		 * One character is for the line break needed.
		 * A second character is for the terminating zero needed.
		 */
		static char payload_buffer[sizeof(counter) * 3 + 1];
		const int length = snprintf(payload_buffer, sizeof(payload_buffer), "%lu\n", counter);
		if (length <= 0) {
			printf("Counter conversion failed\n");
			continue;
		}
		counter++;

		// Flag 0x08 is last_block Flag
		bundle_add_block(bundlemem, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, (uint8_t*)payload_buffer, length);


		const event_container_t send_event = {
			.event = dtn_send_bundle_event,
			.bundlemem = bundlemem
		};
		agent_send_event(&send_event);
	}
}


bool init()
{
	if ( !dtn_process_create(counter_process, "DTN Counter") ) {
		return false;
	}

  return true;
}
