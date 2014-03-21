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
#include "mmem.h"
#include "sys/test.h"
#include "sys/profiling.h"
#include "watchdog.h"
#include "pressure-sensor.h"
#include "leds.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/api.h"
#include "net/uDTN/statistics.h"

#define CONF_SEND_TO_NODE	6
#define CONF_SEND_TO_APP	25
#define CONF_SEND_FROM_NODE	1
#define CONF_SEND_FROM_APP	25
#define CONF_APP_INTERVAL  200
// #define CONF_APP_INTERVAL  1

#ifndef CONF_SEND_TO_NODE
#error "I need a destination node - set CONF_SEND_TO_NODE"
#endif

#ifndef CONF_SEND_TO_APP
#error "I need a destination application - set CONF_SEND_TO_APP"
#endif

#define TYPE_MEASUREMENT	1
#define TYPE_STARTUP		2

/*---------------------------------------------------------------------------*/
PROCESS(temperature_process, "Temperature process");
AUTOSTART_PROCESSES(&temperature_process);
static uint32_t bundles_sent;
/*---------------------------------------------------------------------------*/
int send_bundle(uint8_t * payload, uint8_t length)
{
	uint32_t tmp;
	struct mmem * bundle_out;

	bundle_out = bundle_create_bundle();
	if( bundle_out == NULL ) {
		printf("Cannot create bundle\n");
		return 0;
	}

	/* Source and destination */
	tmp = CONF_SEND_TO_NODE;
	bundle_set_attr(bundle_out, DEST_NODE, &tmp);
	tmp = CONF_SEND_TO_APP;
	bundle_set_attr(bundle_out, DEST_SERV, &tmp);

	tmp = BUNDLE_FLAG_SINGLETON; // Endpoint is Singleton
	bundle_set_attr(bundle_out, FLAGS, &tmp);

	// Lifetime of a quarter of a day
	tmp = 10800;
	bundle_set_attr(bundle_out, LIFE_TIME, &tmp);

	// Add the payload block
	bundle_add_block(bundle_out, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload, length);

	// Submit the bundle to the agent
	process_post(&agent_process, dtn_send_bundle_event, (void *) bundle_out);

	return 1;
}
/*---------------------------------------------------------------------------*/
int send_application_bundle(uint8_t bundle_type)
{
	uint8_t offset = 0;
	uint8_t userdata[10];

	// Create our payload
	userdata[offset++] = bundle_type;

	if( bundle_type == TYPE_MEASUREMENT ) {
		uint16_t tmp = 0;
		// Measurement bundle

		// 4 Bytes Sequence Number
		memcpy(&userdata[offset], &bundles_sent, sizeof(bundles_sent));
		offset += sizeof(bundles_sent);

		// 2 Bytes Temperature
		tmp = pressure_sensor.value(TEMP);
		memcpy(&userdata[offset], &tmp, sizeof(tmp));
		offset += sizeof(tmp);
	} else if( bundle_type == TYPE_STARTUP) {
		uint32_t tmp = 0;

		// Startup bundle

		// 4 Byte timestamp
		tmp = clock_time();
		memcpy(&userdata[offset], &tmp, sizeof(tmp));
		offset += sizeof(tmp);

		// 4 Byte clock resolution
		tmp = CLOCK_SECOND;
		memcpy(&userdata[offset], &tmp, sizeof(tmp));
		offset += sizeof(tmp);
	} else {
		printf("Bundle type %u unknown\n", bundle_type);
	}

	return send_bundle(userdata, offset);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(temperature_process, ev, data)
{
	static struct etimer packet_timer;
	static int create_bundles = 1;
	static struct registration_api reg;
	int ret;

	PROCESS_BEGIN();

	bundles_sent = 0;

	SENSORS_ACTIVATE(pressure_sensor);

	// Turn LEDs on to show, that we are running
	leds_on(LEDS_ALL);

	PROCESS_PAUSE();

	// Turn green LED off to show, that we are running
	leds_off(LEDS_GREEN);

	// Register our application
	reg.status = APP_ACTIVE;
	reg.application_process = PROCESS_CURRENT();
	reg.app_id = 25;
	process_post(&agent_process, dtn_application_registration_event, &reg);

	// Send STARTUP bundle
	send_application_bundle(TYPE_STARTUP);
	PROCESS_WAIT_UNTIL(ev == dtn_bundle_stored || ev == dtn_bundle_store_failed);
	if( ev == dtn_bundle_stored ) {
		printf("APP: STARTUP bundle sent\n");
	} else if( ev == dtn_bundle_store_failed ) {
		printf("APP: STARTUP bundle failed\n");
	}

	// Set the timer for our next data packet
	etimer_set(&packet_timer, CLOCK_SECOND * 10);

	// Turn LEDs off again, so that the agent can use them
	leds_off(LEDS_ALL);

	while(1) {
		PROCESS_WAIT_EVENT();

		if( etimer_expired(&packet_timer) && dtn_node_id == CONF_SEND_FROM_NODE && create_bundles == 1 ) {
			ret = send_application_bundle(TYPE_MEASUREMENT);

			if( ret == 0 ) {
				printf("APP: bundle submission failed\n");

				// Send continue event to ourselves to retry sending the bundle
				process_post(PROCESS_CURRENT(), PROCESS_EVENT_CONTINUE, NULL);
				continue;
			}

			// Increment bundle counter
			bundles_sent++;

			printf("APP: %lu bundles sent\n", bundles_sent);

			// Avoid sending bundles until we have the callback
			create_bundles = 0;
		} else if( etimer_expired(&packet_timer) ) {
			printf("Timer elapsed, cannot send bundles ATM\n");
		}

		/* The last bundle has been saved */
		if( ev == dtn_bundle_stored ) {
			printf("--> OK\n");
			/* The bundle has been processed, so create more bundles */
			create_bundles = 1;
			etimer_set(&packet_timer, CLOCK_SECOND * CONF_APP_INTERVAL);
		}

		/* The last bundle could not be saved */
		if( ev == dtn_bundle_store_failed ) {
			printf("--> FAIL\n");
			/* The bundle has been (unsuccessfully) processed, so create more bundles */
			create_bundles = 1;
			etimer_set(&packet_timer, CLOCK_SECOND * CONF_APP_INTERVAL);
		}

	}

	PROCESS_END();
}
