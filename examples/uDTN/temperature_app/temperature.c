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
#include "button-sensor.h"
#include "leds.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/api.h"
#include "net/uDTN/statistics.h"

#define CONF_SEND_TO_NODE	2
#define CONF_SEND_TO_APP	25
#define CONF_SEND_FROM_NODE	46
#define CONF_SEND_FROM_APP	25
#define CONF_APP_INTERVAL  300
#define DTN_PING_ENDPOINT	11

#ifndef CONF_SEND_TO_NODE
#error "I need a destination node - set CONF_SEND_TO_NODE"
#endif

#ifndef CONF_SEND_TO_NODE
#error "I need a destination application - set CONF_SEND_TO_APP"
#endif

#define TYPE_MEASUREMENT	1
#define TYPE_TIMESYNC		2
#define TYPE_STARTUP		3
#define TYPE_CONTACTS		4
#define TYPE_STATISTICS		5

/*---------------------------------------------------------------------------*/
PROCESS(temperature_process, "Temperature process");
PROCESS(dtnping_process, "DTN Ping process");
AUTOSTART_PROCESSES(&temperature_process, &dtnping_process);
static struct registration_api reg;
struct bundle_t bundle_out;
static uint32_t bundles_sent;
static struct registration_api reg_ping;
/*---------------------------------------------------------------------------*/
void send_bundle(uint8_t * payload, uint8_t length)
{
	uint32_t tmp;
	struct mmem * bundle_out;

	bundle_out = bundle_create_bundle();
	if( bundle_out == NULL ) {
		printf("Cannot create bundle\n");
		return;
	}

	/* Source and destination */
	tmp = CONF_SEND_TO_NODE;
	bundle_set_attr(bundle_out, DEST_NODE, &tmp);
	tmp = CONF_SEND_TO_APP;
	bundle_set_attr(bundle_out, DEST_SERV, &tmp);

	tmp = CONF_SEND_FROM_APP;
	bundle_set_attr(bundle_out, SRC_SERV, &tmp);

	tmp = BUNDLE_FLAG_SINGLETON; // Endpoint is Singleton
	bundle_set_attr(bundle_out, FLAGS, &tmp);

	// Lifetime of a full day
	tmp = 86400;
	bundle_set_attr(bundle_out, LIFE_TIME, &tmp);

	/**
	 * Hardcoded creation timestamp based on:
	 * date -j +%s   -    date -j 010100002000 +%s
	 */
	tmp = 388152261 + clock_seconds();
	bundle_set_attr(bundle_out, TIME_STAMP, &tmp);

	// Add the payload block
	bundle_add_block(bundle_out, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload, length);

	// Submit the bundle to the agent
	process_post(&agent_process, dtn_send_bundle_event, (void *) bundle_out);
}
/*---------------------------------------------------------------------------*/
void send_application_bundle(uint8_t bundle_type)
{
	uint8_t offset;
	uint32_t tmp;
	uint8_t userdata[80];

	// Create our payload
	offset = 0;
	userdata[offset++] = bundle_type;

	if( bundle_type == TYPE_MEASUREMENT ) {
		// Measurement bundle
		userdata[offset++] = (bundles_sent & 0x0000FF) >>  0;
		userdata[offset++] = (bundles_sent & 0x00FF00) >>  8;
		userdata[offset++] = (bundles_sent & 0xFF0000) >> 16;

		tmp = clock_seconds();
		userdata[offset++] = (tmp & 0x000000FF) >>  0;
		userdata[offset++] = (tmp & 0x0000FF00) >>  8;
		userdata[offset++] = (tmp & 0x00FF0000) >> 16;

		tmp = pressure_sensor.value(TEMP);
		userdata[offset++] = (tmp & 0x00FF) >> 0;
		userdata[offset++] = (tmp & 0xFF00) >> 8;
	} else if( bundle_type == TYPE_TIMESYNC) {
		// Timesync bundle
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
	} else if( bundle_type == TYPE_STARTUP) {
		// Startup bundle
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
	} else {
		printf("Bundle type %u unknown\n", bundle_type);
	}

	send_bundle(userdata, offset);
}
/*---------------------------------------------------------------------------*/
void send_contacts_bundle()
{
	uint8_t userdata[81];
	uint8_t length;
	userdata[0] = TYPE_CONTACTS;

	length = statistics_get_contacts_bundle(userdata + 1, 80);

	send_bundle(userdata, length + 1);
}
/*---------------------------------------------------------------------------*/
void send_statistics_bundle()
{
	uint8_t userdata[81];
	uint8_t length;
	userdata[0] = TYPE_STATISTICS;

	length = statistics_get_bundle(userdata + 1, 80);

	send_bundle(userdata, length + 1);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(temperature_process, ev, data)
{
	static struct etimer packet_timer;
	static struct etimer statistics_timer;
	static struct etimer contacts_timer;

	PROCESS_BEGIN();

	bundles_sent = 0;

	SENSORS_ACTIVATE(pressure_sensor);
	SENSORS_ACTIVATE(button_sensor);

	// Turn LEDs on to show, that we are running
	leds_on(LEDS_ALL);

	PROCESS_PAUSE();

	// Turn green LED off to show, that we are running
	leds_off(LEDS_GREEN);

	reg.status = APP_ACTIVE;
	reg.application_process = PROCESS_CURRENT();
	reg.app_id = 25;
	process_post(&agent_process, dtn_application_registration_event, &reg);

	// Initialize the statistics module and set a timer
	uint16_t interval = statistics_setup(&temperature_process);
	etimer_set(&statistics_timer, CLOCK_SECOND * interval);

	// Wait a second to send our STARTUP bundle
	etimer_set(&packet_timer, CLOCK_SECOND);
	PROCESS_YIELD_UNTIL(etimer_expired(&packet_timer));

	// Send STARTUP bundle
	send_application_bundle(TYPE_STARTUP);
	printf("APP: STARTUP bundle sent\n");

	// Set the timer for our next data packet
	etimer_set(&packet_timer, CLOCK_SECOND * CONF_APP_INTERVAL);
	etimer_set(&contacts_timer, CLOCK_SECOND * 3600 * 4);

	// Turn LEDs off again, so that the agent can use them
	leds_off(LEDS_ALL);

	while(1) {
		PROCESS_YIELD();

		if( etimer_expired(&statistics_timer)) {
			send_statistics_bundle();
			statistics_reset();

			printf("APP: STATISTICS bundle sent\n");

			etimer_reset(&statistics_timer);

			continue;
		}

		if( ev == sensors_event && data == &button_sensor ) {
			// Button pressed, next bundle will be timesync
			send_application_bundle(TYPE_TIMESYNC);
			printf("APP: TIMESYNC bundle sent\n");
			continue;
		}

		if( etimer_expired(&packet_timer) && dtn_node_id == CONF_SEND_FROM_NODE ) {
			printf("APP: %lu bundles sent\n", bundles_sent);
			send_application_bundle(TYPE_MEASUREMENT);
			bundles_sent++;

			// Send the next bundle after CONF_APP_INTERVAL seconds
			etimer_set(&packet_timer, CLOCK_SECOND * CONF_APP_INTERVAL);
		}

		if( ev == submit_data_to_application_event ) {
			// Bundle has arrived
			struct mmem * bundlemem = (struct mmem *) data;
			struct bundle_block_t *block;

			/* Get the payload block */
			block = bundle_get_payload_block(bundlemem);
			if( block == NULL ) {
				printf("Bundle without payload block\n");
				continue;
			}

			printf("APP: Payload (%u): ", block->block_size);
			int i;
			for(i=0; i<block->block_size; i++) {
				printf("%02X ", block->payload[i]);
			}
			printf("\n");

			/* Tell the agent, that we have processed the bundle */
			process_post(&agent_process, dtn_processing_finished, bundlemem);

			/* NULLify the pointer */
			bundlemem = NULL;
			block = NULL;

			continue;
		}

		if( ev == dtn_statistics_overrun || etimer_expired(&contacts_timer) ) {
			// Contacts statistics are running over, send them out
			send_contacts_bundle();
			statistics_reset_contacts();

			printf("APP: CONTACTS bundle sent\n");

			etimer_restart(&contacts_timer);

			continue;
		}

	}

	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dtnping_process, ev, data)
{
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

	PROCESS_PAUSE();

	reg_ping.status = APP_ACTIVE;
	reg_ping.application_process = &dtnping_process;
	reg_ping.app_id = DTN_PING_ENDPOINT;
	process_post(&agent_process, dtn_application_registration_event, &reg_ping);

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == submit_data_to_application_event);

		// Reconstruct the bundle struct from the event
		bundlemem = (struct mmem *) data;
		bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

		// preserve the payload to send it back
		uint8_t payload_buffer[64];
		uint8_t payload_length;

		block = bundle_get_payload_block(bundlemem);
		payload_length = block->block_size;
		if (payload_length > 64) {
			printf("PING: Payload too big, clamping to maximum size.\n");
		}
		memcpy(payload_buffer, block->payload, payload_length);

		// Extract the source information to send a reply back
		bundle_get_attr(bundlemem, SRC_NODE, &source_node);
		bundle_get_attr(bundlemem, SRC_SERV, &source_service);

		// Extract timestamp and lifetime from incoming bundle
		bundle_get_attr(bundlemem, TIME_STAMP, &incoming_timestamp);
		bundle_get_attr(bundlemem, LIFE_TIME, &incoming_lifetime);

		// Tell the agent, that have processed the bundle
		process_post(&agent_process, dtn_processing_finished, bundlemem);

		bundles_recv++;
		printf("PING: %lu received\n", bundles_recv);

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
		bundle_set_attr(bundlemem, TIME_STAMP, &incoming_timestamp);

		// Copy payload from incoming bundle
		bundle_add_block(bundlemem, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload_buffer, payload_length);

		// And submit the bundle to the agent
		process_post(&agent_process, dtn_send_bundle_event, (void *) bundlemem);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
