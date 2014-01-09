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
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "sys/test.h"
#include "sys/profiling.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/api.h"

/*---------------------------------------------------------------------------*/
PROCESS(udtn_demo_process, "uDTN demo process");
AUTOSTART_PROCESSES(&udtn_demo_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udtn_demo_process, ev, data)
{
	static struct registration_api reg;
	struct mmem * outgoing_bundle_memory = NULL;
	uint8_t payload[255];
	uint32_t tmp;

	PROCESS_BEGIN();

	/* Wait for the agent to be initialized */
	PROCESS_PAUSE();

	/* Register our endpoint to receive bundles */
	reg.status = APP_ACTIVE;
	reg.application_process = PROCESS_CURRENT();
	reg.app_id = 25;
	process_post(&agent_process, dtn_application_registration_event,&reg);

	/* Allocate memory for the outgoing bundle */
	outgoing_bundle_memory = bundle_create_bundle();
	if( outgoing_bundle_memory == NULL ) {
		printf("No memory to send bundle\n");
		return -1;
	}

	/* Get the bundle flags */
	bundle_get_attr(outgoing_bundle_memory, FLAGS, &tmp);

	/* Set the bundle flags to singleton */
	tmp = BUNDLE_FLAG_SINGLETON;
	bundle_set_attr(outgoing_bundle_memory, FLAGS, &tmp);

	/* Add the payload block */
	bundle_add_block(outgoing_bundle_memory, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload, 255);

	/* To send the bundle, we send an event to the agent */
	process_post(&agent_process, dtn_send_bundle_event, (void *) outgoing_bundle_memory);

	TEST_PASS();

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
