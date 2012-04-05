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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"

#include "net/netstack.h"
#include "net/packetbuf.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/API_events.h"
#include "net/uDTN/API_registration.h"

#include "net/uDTN/dtn_config.h"
#include "net/uDTN/storage.h"
#include "mmem.h"
#include "sys/test.h"
#include "sys/profiling.h"
#include "watchdog.h"
#include "pressure-sensor.h"
#include "gyro-sensor.h"
#include "button-sensor.h"

#define CONF_SEND_TO_NODE	 2
#define CONF_SEND_TO_APP	25
#define CONF_SEND_FROM_APP	25
#define FORMAT_BINARY		 1

#ifndef CONF_SEND_TO_NODE
#error "I need a destination node - set CONF_SEND_TO_NODE"
#endif

#ifndef CONF_SEND_TO_NODE
#error "I need a destination application - set CONF_SEND_TO_APP"
#endif

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
static struct registration_api reg;
struct bundle_t bundle;
static uint32_t sequence_number;
static uint32_t bundles_sent;
/*---------------------------------------------------------------------------*/
void send_bundle(uint8_t bundle_type)
{
	uint32_t tmp;
	uint8_t offset;
	uint8_t userdata[80];

	create_bundle(&bundle);

	/* Source and destination */
	tmp = CONF_SEND_TO_NODE;
	set_attr(&bundle, DEST_NODE, &tmp);
	tmp = CONF_SEND_TO_APP;
	set_attr(&bundle, DEST_SERV, &tmp);

	tmp = dtn_node_id;
	set_attr(&bundle, SRC_NODE, &tmp);
	tmp = CONF_SEND_FROM_APP;
	set_attr(&bundle, SRC_SERV,&tmp);

	tmp = 0;
	set_attr(&bundle, CUST_NODE, &tmp);
	set_attr(&bundle, CUST_SERV, &tmp);
	set_attr(&bundle, REP_NODE, &tmp);
	set_attr(&bundle, REP_SERV, &tmp);

	tmp = 0x10; // Endpoint is Singleton
	set_attr(&bundle, FLAGS, &tmp);

	/* Set the sequence number to the number of bundles sent */
	tmp = sequence_number ++;
	set_attr(&bundle, TIME_STAMP_SEQ_NR, &tmp);

	// Infinite lifetime
	tmp = 86400;
	set_attr(&bundle, LIFE_TIME, &tmp);

	// Zero timestamp
	tmp = 386954881 + (clock_time() / CLOCK_SECOND);
	set_attr(&bundle, TIME_STAMP, &tmp);

	// Create our payload
	offset = 0;

	if( bundle_type == 1 ) {
		// Measurement bundle
#if FORMAT_BINARY
		userdata[offset++] = (bundles_sent & 0x0000FF) >>  0;
		userdata[offset++] = (bundles_sent & 0x00FF00) >>  8;
		userdata[offset++] = (bundles_sent & 0xFF0000) >> 16;

		tmp = clock_time();
		userdata[offset++] = (tmp & 0x000000FF) >>  0;
		userdata[offset++] = (tmp & 0x0000FF00) >>  8;
		userdata[offset++] = (tmp & 0x00FF0000) >> 16;

		tmp = pressure_sensor.value(TEMP);
		userdata[offset++] = (tmp & 0x00FF) >> 0;
		userdata[offset++] = (tmp & 0xFF00) >> 8;

		userdata[offset++] = gyro_sensor.value(TEMP_AS);
#else
		offset += sprintf((char *) userdata + offset, "%lu ", bundles_sent);
		offset += sprintf((char *) userdata + offset, "%lu ", clock_time() / CLOCK_SECOND);
		offset += sprintf((char *) userdata + offset, "%u ", pressure_sensor.value(TEMP));
		offset += sprintf((char *) userdata + offset, "%u ", gyro_sensor.value(TEMP_AS));
		offset += sprintf((char *) userdata + offset, "\n");
#endif
	} else if( bundle_type == 2) {
		// Timesync bundle
#if FORMAT_BINARY
		userdata[offset++] = 0x12;
		userdata[offset++] = 0x34;
		userdata[offset++] = 0x56;
		userdata[offset++] = 0x78;

		tmp = clock_time();
		userdata[offset++] = (tmp & 0x000000FF) >>  0;
		userdata[offset++] = (tmp & 0x0000FF00) >>  8;
		userdata[offset++] = (tmp & 0x00FF0000) >> 16;
		userdata[offset++] = (tmp & 0xFF000000) >> 24;

		tmp = CLOCK_SECOND;
		userdata[offset++] = (tmp & 0x000000FF) >>  0;
		userdata[offset++] = (tmp & 0x0000FF00) >>  8;
		userdata[offset++] = (tmp & 0x00FF0000) >> 16;
		userdata[offset++] = (tmp & 0xFF000000) >> 24;

		userdata[offset++] = 0x90;
#else
		offset = sprintf((char *) userdata, "TIMESYNC %lu %lu\n", clock_time(), CLOCK_SECOND);
#endif
	} else if( bundle_type == 3) {
		// Startup bundle
#if FORMAT_BINARY
		userdata[offset++] = 0x78;
		userdata[offset++] = 0x56;
		userdata[offset++] = 0x34;
		userdata[offset++] = 0x12;

		tmp = clock_time();
		userdata[offset++] = (tmp & 0x000000FF) >>  0;
		userdata[offset++] = (tmp & 0x0000FF00) >>  8;
		userdata[offset++] = (tmp & 0x00FF0000) >> 16;
		userdata[offset++] = (tmp & 0xFF000000) >> 24;

		tmp = CLOCK_SECOND;
		userdata[offset++] = (tmp & 0x000000FF) >>  0;
		userdata[offset++] = (tmp & 0x0000FF00) >>  8;
		userdata[offset++] = (tmp & 0x00FF0000) >> 16;
		userdata[offset++] = (tmp & 0xFF000000) >> 24;

		userdata[offset++] = 0x09;
#else
		offset = sprintf((char *) userdata, "STARTUP %lu %lu\n", clock_time(), CLOCK_SECOND);
#endif
	} else {
		printf("Bundle type %u unknown\n", bundle_type);
	}

	// Add the payload block
	add_block(&bundle, 1, 0x08, userdata, offset);

	// Submit the bundle to the agent
	process_post(&agent_process, dtn_send_bundle_event, (void *) &bundle);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
	static struct etimer timer;
	static uint8_t cnt = 0;
	static uint8_t bundle_type = 3;

	PROCESS_BEGIN();

	sequence_number = 0;
	bundles_sent = 0;

	SENSORS_ACTIVATE(pressure_sensor);
	SENSORS_ACTIVATE(gyro_sensor);
	SENSORS_ACTIVATE(button_sensor);

	agent_init();
	reg.status = 1;
	reg.application_process=&hello_world_process;
	reg.app_id = 25;

	etimer_set(&timer,  CLOCK_SECOND*1);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	process_post(&agent_process, dtn_application_registration_event,&reg);

	etimer_set(&timer,  CLOCK_SECOND);

	while(1) {
		PROCESS_YIELD();

		if(ev == sensors_event && data == &button_sensor && bundle_type != 3) {
			// Button pressed, next bundle will be timesync
			bundle_type = 2;
		}

		send_bundle(bundle_type);

		if( bundle_type == 1 ) {
			bundles_sent++;
			printf("%lu bundles sent\n", bundles_sent);
		} else if( bundle_type == 2 ) {
			printf("TIMESYNC bundle sent\n");
		} else if( bundle_type == 3) {
			printf("STARTUP bundle sent\n");
		}

		// Send the next bundle after 1s
		etimer_set(&timer,  CLOCK_SECOND * 300);

		// Next bundle will be a regular data bundle
		bundle_type = 1;
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
