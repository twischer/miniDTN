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
#include "button-sensor.h"
#include "net/uDTN/API_registration.h"
#include "net/uDTN/API_events.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/sdnv.h"
#include "statistics.h"
#include "leds.h"

#include "r_storage.h"

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

	create_bundle(&bundle_out);

	/* Source and destination */
	tmp = CONF_SEND_TO_NODE;
	set_attr(&bundle_out, DEST_NODE, &tmp);
	tmp = CONF_SEND_TO_APP;
	set_attr(&bundle_out, DEST_SERV, &tmp);

	tmp = dtn_node_id;
	set_attr(&bundle_out, SRC_NODE, &tmp);
	tmp = CONF_SEND_FROM_APP;
	set_attr(&bundle_out, SRC_SERV,&tmp);

	tmp = 0;
	set_attr(&bundle_out, CUST_NODE, &tmp);
	set_attr(&bundle_out, CUST_SERV, &tmp);
	set_attr(&bundle_out, REP_NODE, &tmp);
	set_attr(&bundle_out, REP_SERV, &tmp);

	tmp = 0x10; // Endpoint is Singleton
	set_attr(&bundle_out, FLAGS, &tmp);

	// Lifetime of a full day
	tmp = 86400;
	set_attr(&bundle_out, LIFE_TIME, &tmp);

	/**
	 * Hardcoded creation timestamp based on:
	 * date -j +%s   -    date -j 010100002000 +%s
	 */
	tmp = 388152261 + clock_seconds();
	set_attr(&bundle_out, TIME_STAMP, &tmp);

	// Add the payload block
	add_block(&bundle_out, 1, 0, payload, length);

	// Submit the bundle to the agent
	process_post(&agent_process, dtn_send_bundle_event, (void *) &bundle_out);
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

	agent_init();

	// Turn LEDs on to show, that we are running
	leds_on(LEDS_ALL);

	etimer_set(&packet_timer, CLOCK_SECOND);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&packet_timer));

	// Turn green LED off to show, that we are running
	leds_off(LEDS_GREEN);

	reg.status = 1;
	reg.application_process = &temperature_process;
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
			struct bundle_t * bundle = (struct bundle_t *) data;
			struct bundle_block_t *block;

			uint8_t payload_buffer[80];

			block = get_block(bundle);
			memcpy(payload_buffer, MMEM_PTR(&block->payload), block->block_size);

			delete_bundle(bundle);

			printf("APP: Payload (%u): ", block->block_size);
			int i;
			for(i=0; i<block->block_size; i++) {
				printf("%02X ", payload_buffer[i]);
			}
			printf("\n");

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
	static struct etimer timer;
	static uint32_t bundles_recv = 0;
	uint32_t tmp;

	uint32_t source_node;
	uint32_t source_service;
	uint32_t incoming_timestamp;
	uint32_t incoming_lifetime;

	PROCESS_BEGIN();

	etimer_set(&timer, CLOCK_SECOND*1);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	reg_ping.status = 1;
	reg_ping.application_process = &dtnping_process;
	reg_ping.app_id = DTN_PING_ENDPOINT;
	process_post(&agent_process, dtn_application_registration_event, &reg_ping);

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == submit_data_to_application_event);

		// Reconstruct the bundle struct from the event
		struct bundle_t * bundle_in = (struct bundle_t *) data;
		struct bundle_block_t *block_in;

		// preserve the payload to send it back
		uint8_t payload_buffer[64];
		uint8_t payload_length;
		uint8_t offset;

		block_in = get_block(bundle_in);
		payload_length = block_in->block_size;

		// Safeguard agains buffer overflow
		if (payload_length > 64) {
			printf("Error, buffer too small! Clamping payload\n");
			payload_length = 64;
		}

		memcpy(payload_buffer, MMEM_PTR(&block_in->payload), payload_length);

		// Extract the source information to send a reply back
		get_attr(bundle_in, SRC_NODE, &source_node);
		get_attr(bundle_in, SRC_SERV, &source_service);

		// Extract timestamp and lifetime from incoming bundle
		get_attr(bundle_in, TIME_STAMP, &incoming_timestamp);
		get_attr(bundle_in, LIFE_TIME, &incoming_lifetime);

		// Delete the incoming bundle
		delete_bundle(bundle_in);

		bundles_recv++;
        uint32_t seqno = (((uint32_t) (payload_buffer[0] & 0xFF)) <<  0) +
                         (((uint32_t) (payload_buffer[1] & 0xFF)) <<  8) +
                         (((uint32_t) (payload_buffer[2] & 0xFF)) << 16) +
                         (((uint32_t) (payload_buffer[3] & 0xFF)) << 24);

		printf("PING: PING %lu (SeqNo %lu) received\n", bundles_recv, seqno);

		// Create the reply bundle
		create_bundle(&bundle_out);

		// Set the reply EID to the incoming bundle information
		set_attr(&bundle_out, DEST_NODE, &source_node);
		set_attr(&bundle_out, DEST_SERV, &source_service);

		// Make us the sender, the custodian and the report to
		tmp = dtn_node_id;
		set_attr(&bundle_out, SRC_NODE, &tmp);
		set_attr(&bundle_out, CUST_NODE, &tmp);
		set_attr(&bundle_out, CUST_SERV, &tmp);
		set_attr(&bundle_out, REP_NODE, &tmp);
		set_attr(&bundle_out, REP_SERV, &tmp);

		// Set our service to 11 [DTN_PING_ENDPOINT] (IBR-DTN expects that)
		tmp = DTN_PING_ENDPOINT;
		set_attr(&bundle_out, SRC_SERV, &tmp);

		// Now set the flags
		tmp = 0x10; // Endpoint is Singleton
		set_attr(&bundle_out, FLAGS, &tmp);

		// Set the same lifetime and timestamp as the incoming bundle
		set_attr(&bundle_out, LIFE_TIME, &incoming_lifetime);
		set_attr(&bundle_out, TIME_STAMP, &incoming_timestamp);

		// Copy payload from incoming bundle
		// Flag 0x08 is last_block Flag, add_block takes care of this
		add_block(&bundle_out, 1, 0, payload_buffer, payload_length);

		// And submit the bundle to the agent
		process_post(&agent_process, dtn_send_bundle_event, (void *) &bundle_out);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
