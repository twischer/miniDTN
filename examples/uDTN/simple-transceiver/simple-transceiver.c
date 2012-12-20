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
 *         A simple service that sends bundles over uDTN
 * \author
 *         Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "process.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/api.h"

/*---------------------------------------------------------------------------*/
#ifndef CONF_SEND_TO_NODE
#define CONF_SEND_TO_NODE 		dtn_node_id
#endif
#ifndef CONF_SEND_TO_SERVICE
#define CONF_SEND_TO_SERVICE 	15
#endif
/*---------------------------------------------------------------------------*/
PROCESS(simple_transceiver_process, "Simple Sender process");
AUTOSTART_PROCESSES(&simple_transceiver_process);
/*---------------------------------------------------------------------------*/
int send_bundle(uint32_t destination_node, uint32_t destination_service, uint8_t * payload, uint8_t length)
{
	struct mmem * outgoing_bundle_memory = NULL;
	uint32_t tmp;
	int n;

	/* Allocate memory for the outgoing bundle */
	outgoing_bundle_memory = bundle_create_bundle();
	if( outgoing_bundle_memory == NULL ) {
		printf("No memory to send bundle\n");
		return -1;
	}

	/* Set the destination node and service */
	bundle_set_attr(outgoing_bundle_memory, DEST_NODE, &destination_node);
	bundle_set_attr(outgoing_bundle_memory, DEST_SERV, &destination_service);

	/* The endpoint is a singleton endpoint */
	tmp = BUNDLE_FLAG_SINGLETON;
	bundle_set_attr(outgoing_bundle_memory, FLAGS, &tmp);

	/* Lifetime is a full hour */
	tmp = 3600;
	bundle_set_attr(outgoing_bundle_memory, LIFE_TIME, &tmp);

	/* Add the payload block */
	n = bundle_add_block(outgoing_bundle_memory, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload, length);
	if( !n ) {
		printf("Not enough memory for payload block\n");

		/* If anything goes wrong, we have to free memory */
		bundle_decrement(outgoing_bundle_memory);

		return -1;
	}

	/* To send the bundle, we send an event to the agent */
	process_post(&agent_process, dtn_send_bundle_event, (void *) outgoing_bundle_memory);

	return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(simple_transceiver_process, ev, data)
{
	static struct registration_api registration;
	static struct etimer timer;
	static int create_bundles;
	int n;

	PROCESS_BEGIN();

	/* Wait for the agent to be initialized */
	PROCESS_PAUSE();

	/* Register our endpoint to send and receive bundles */
	registration.status = APP_ACTIVE;
	registration.application_process = PROCESS_CURRENT();
	registration.app_id = 15;
	process_post(&agent_process, dtn_application_registration_event, &registration);

	/* Set a timer */
	etimer_set(&timer, CLOCK_SECOND * 6);

	/* And enable bundle creation */
	create_bundles = 1;

	/* Continously send bundles */
	while(1) {
		PROCESS_WAIT_EVENT();

		/* Send the next bundle */
		if( etimer_expired(&timer) && create_bundles ) {
			/* We need payload */
			uint8_t payload[40];
			memset(payload, 0, 40);

			printf("Sending bundle to ipn:%lu.%d\n", CONF_SEND_TO_NODE, CONF_SEND_TO_SERVICE);

			/* Send out the bundle with our payload */
			n = send_bundle(CONF_SEND_TO_NODE, CONF_SEND_TO_SERVICE, payload, 40);

			if( n ) {
				/* Whenever we have sent a bundle, we wait for feedback from the agent */
				create_bundles = 0;
			} else {
				/* Otherwise we just reschedule and send again */
				etimer_reset(&timer);
			}
		}

		/* The last bundle has been saved */
		if( ev == dtn_bundle_stored ) {
			/* The bundle has been processed, so create more bundles */
			create_bundles = 1;
			etimer_restart(&timer);
		}

		/* The last bundle could not be saved */
		if( ev == dtn_bundle_store_failed ) {
			/* The bundle has been (unsuccessfully) processed, so create more bundles */
			create_bundles = 1;
			etimer_restart(&timer);
		}

		/* Incoming bundle */
		if( ev == submit_data_to_application_event ) {
			/* data contains a pointer to the bundle memory */
			struct mmem * incoming_bundle_memory = (struct mmem *) data;

			/* with MMEM_PTR we can reconstruct the original bundle structure */
			struct bundle_t * incoming_bundle = (struct bundle_t *) MMEM_PTR(incoming_bundle_memory);

			/* We can read several attributes as defined in bundle.h */
			uint32_t source_node;
			uint32_t source_service;
			bundle_get_attr(incoming_bundle_memory, SRC_NODE, &source_node);
			bundle_get_attr(incoming_bundle_memory, SRC_SERV, &source_service);

			/* We can obtain the bundle payload block like so: */
			struct bundle_block_t * block = bundle_get_payload_block(incoming_bundle_memory);

			printf("Bundle received from ipn:%lu.%lu with %u bytes\n", source_node, source_service, block->block_size);

			/* After processing the bundle, we have to notify the agent to free up the memory */
			process_post(&agent_process, dtn_processing_finished, incoming_bundle_memory);

			/* Afterwards, we should void our pointer to not get confused */
			incoming_bundle_memory = NULL;
			incoming_bundle = NULL;
		}

	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
