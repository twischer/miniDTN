/*
 * Copyright (c) 2012, Daniel Willmann <daniel@totalueberwachung.de>
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
 */

/**
 * \file
 *         A uDTN ping-pong example testing end-to-end latency
 * \author
 *         Daniel Willmann <daniel@totalueberwachung.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "watchdog.h"
#include "sys/test.h"
#include "sys/profiling.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/sdnv.h"
#include "net/uDTN/api.h"
#include "net/uDTN/storage.h"

#define MODE_PASSIVE 0
#define MODE_ACTIVE 1
#define MODE_LOOPBACK 2

#ifndef CONF_MODE
#warning "CONF_MODE not defined, this node will operate in loopback mode"
#define CONF_MODE MODE_LOOPBACK
#endif

#if CONF_MODE == MODE_LOOPBACK
#undef CONF_DEST_NODE
#define CONF_DEST_NODE dtn_node_id
#endif

#ifndef CONF_DEST_NODE
#error "I need a destination node - set CONF_DEST_NODE"
#endif

#ifdef CONF_PAYLOAD_LEN
#if CONF_PAYLOAD_LEN < 4
#error "Payload must be at least 4 bytes big"
#endif
#define PAYLOAD_LEN CONF_PAYLOAD_LEN
#else
#define PAYLOAD_LEN 80
#endif

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
PROCESS(ping_process, "Ping");
PROCESS(pong_process, "Pong");
PROCESS(coordinator_process, "Coordinator");

AUTOSTART_PROCESSES(&coordinator_process);

struct bundle_t bundle;
/*---------------------------------------------------------------------------*/

static clock_time_t get_time()
{
	return clock_time();
}

/* Convenience function to populate a bundle */
static inline struct mmem *bundle_convenience(uint16_t dest, uint16_t dst_srv, uint16_t src_srv,  uint8_t *data, size_t len)
{
	uint32_t tmp;
	struct mmem *bundlemem;

	bundlemem = bundle_create_bundle();
	if (!bundlemem) {
		printf("create_bundle failed\n");
		return NULL;
	}

	/* Destination node and service */
	tmp=dest;
	bundle_set_attr(bundlemem, DEST_NODE, &tmp);
	tmp=dst_srv;
	bundle_set_attr(bundlemem, DEST_SERV, &tmp);

	/* Source Service */
	tmp=src_srv;
	bundle_set_attr(bundlemem, SRC_SERV, &tmp);

	/* Bundle flags */
	tmp=BUNDLE_FLAG_SINGLETON;
	bundle_set_attr(bundlemem, FLAGS, &tmp);

	/* Bundle lifetime */
	tmp=2000;
	bundle_set_attr(bundlemem, LIFE_TIME, &tmp);

	/* Bundle payload block */
	bundle_add_block(bundlemem, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, data, len);

	return bundlemem;
}

PROCESS_THREAD(coordinator_process, ev, data)
{
	static struct etimer timer;
	PROCESS_BEGIN();

	profiling_init();
	profiling_start();

	PROCESS_PAUSE();

	printf("Starting tests\n");

#if CONF_MODE == MODE_ACTIVE
	process_start(&ping_process, NULL);
#elif CONF_MODE == MODE_PASSIVE
	process_start(&pong_process, NULL);
#else
	process_start(&ping_process, NULL);
	process_start(&pong_process, NULL);
#endif

	/* The pong process doesn't collect anything, so PASS device directly.
	 * Otherwise wait for ping to finish. */
	PROCESS_WAIT_UNTIL(!process_is_running(&ping_process));

	etimer_set(&timer, CLOCK_SECOND*10);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));

	profiling_stop();
	watchdog_stop();
	profiling_report("pingpong", 0);
	watchdog_start();
	TEST_PASS();

	PROCESS_END();
}

PROCESS_THREAD(ping_process, ev, data)
{
	static uint16_t timeouts = 0, bundle_sent = 0, bundle_recvd = 0;
	static uint32_t diff = 0, latency = 0;
	static uint8_t synced = 0;
	static struct etimer timer;
	struct mmem *bundlemem, *recv;
	struct bundle_block_t *block;
	static struct registration_api reg_ping;
	uint8_t i;
	uint8_t userdata[PAYLOAD_LEN];
	uint32_t *u32_ptr;
	uint8_t sent = 0;

	PROCESS_BEGIN();

	/* Register our endpoint */
	reg_ping.status = APP_ACTIVE;
	reg_ping.application_process = PROCESS_CURRENT();
	reg_ping.app_id = 5;
	process_post(&agent_process, dtn_application_registration_event, &reg_ping);

	/* Wait a second */
	etimer_set(&timer,  CLOCK_SECOND);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	printf("PING: Init done, starting test\n");

	/* Transfer */
	while(1) {
		etimer_set(&timer, CLOCK_SECOND*5);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer) ||
				ev == submit_data_to_application_event);

		if (etimer_expired(&timer)) {
			u32_ptr = (uint32_t *)&userdata[0];

			/* Sync pattern */
			if (!synced) {
				if( sent )
					printf("PING: Timeout waiting for sync\n");
				sent = 1;
				*u32_ptr = 0xfdfdfdfd;
			} else {
				timeouts++;
				printf("PING: Timeouts: %u\n", timeouts);
				*u32_ptr = get_time();
			}

			for (i=4;i<PAYLOAD_LEN;i++) {
				userdata[i] = i;
			}

			PRINTF("PING: send sync\n");
			bundlemem = bundle_convenience(CONF_DEST_NODE, 7, 5, userdata, PAYLOAD_LEN);
			if (bundlemem)
				process_post(&agent_process, dtn_send_bundle_event, (void *) bundlemem);

		}

		if( ev == submit_data_to_application_event ) {
			/* We received a bundle - handle it */
			recv = (struct mmem *) data;

			PRINTF("PING: recv\n");

			diff = get_time();

			/* Check receiver */
			block = bundle_get_payload_block(recv);

			if( block == NULL ) {
				printf("PING: No Payload\n");
			} else {
				u32_ptr = (uint32_t *)block->payload;

				if (!synced) {
					/* We're synced */
					if (*u32_ptr == 0xfdfdfdfd) {
						synced = 1;
						printf("PING: synced\n");
					} else {
						printf("PING: received initial bundle but with broken payload\n");

						// Tell the agent, that we have processed the bundle
						process_post(&agent_process, dtn_processing_finished, recv);

						continue;
					}
				} else {
					/* Calculate RTT */
					bundle_recvd++;
					diff -= *u32_ptr;
					// printf("Latency: %lu\n", diff);

					if (bundle_recvd % 50 == 0)
						printf("PING: %u\n", bundle_recvd);

					latency += diff;
				}
			}

			// Tell the agent, that we have processed the bundle
			process_post(&agent_process, dtn_processing_finished, recv);

			/* We're done */
			if (bundle_recvd >= 1000)
				break;

			/* Send PING */
			u32_ptr = (uint32_t *)&userdata[0];
			*u32_ptr = get_time();
			for (i=4;i<PAYLOAD_LEN;i++) {
				userdata[i] = i;
			}

			PRINTF("PING: send ping\n");
			bundlemem = bundle_convenience(CONF_DEST_NODE, 7, 5, userdata, PAYLOAD_LEN);
			if (bundlemem) {
				process_post(&agent_process, dtn_send_bundle_event, (void *) bundlemem);
				bundle_sent++;
			}
		}
	}

	TEST_REPORT("timeout", timeouts, bundle_sent, "lost/sent");
	TEST_REPORT("average latency", latency*1000/bundle_recvd, CLOCK_SECOND, "ms");

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(pong_process, ev, data)
{
	static struct etimer timer;
	static uint16_t bundle_sent = 0;
	struct mmem *bundlemem, *recv;
	struct bundle_block_t *block;
	static struct registration_api reg_pong;
	uint32_t *u32_ptr, tmp;

	PROCESS_BEGIN();

	/* Register our endpoint */
	reg_pong.status = APP_ACTIVE;
	reg_pong.application_process = PROCESS_CURRENT();
	reg_pong.app_id = 7;
	process_post(&agent_process, dtn_application_registration_event, &reg_pong);

	/* Wait a second */
	etimer_set(&timer,  CLOCK_SECOND);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	printf("PONG: Init done, starting responder\n");

	/* Transfer */
	while(1) {
		/* Wait for incoming bundle */
		PROCESS_WAIT_EVENT_UNTIL(ev == submit_data_to_application_event);

		/* We received a bundle - bounce it back */
		recv = (struct mmem *) data;

		/* Find the source node */
		bundle_get_attr(recv, SRC_NODE, &tmp);

		/* Check sender */
		if (tmp != CONF_DEST_NODE) {
			printf("PONG: Bundle from different node.\n");

			// Tell the agent, that we have processed the bundle
			process_post(&agent_process, dtn_processing_finished, recv);

			continue;
		}

		PRINTF("PONG: recv\n");

		/* Verify the content of the bundle */
		block = bundle_get_payload_block(recv);
		int i;

		if( block == NULL ) {
			printf("PONG: Payload: no block\n");
		} else {
			if( block->block_size != PAYLOAD_LEN ) {
				printf("PONG: Payload: length is %d, should be %d\n", block->block_size, PAYLOAD_LEN);
			}

			for(i=4; i<PAYLOAD_LEN; i++) {
				if( block->payload[i] != i ) {
					printf("PONG: Payload: byte %d mismatch. Should be %02X, is %02X\n", i, i, block->payload[i]);
				}
			}
		}

		u32_ptr = (uint32_t *) block->payload;

		/* Tell the agent, that have processed the bundle */
		process_post(&agent_process, dtn_processing_finished, recv);

		/* Send PONG */
		PRINTF("PONG: send\n");
		bundlemem = bundle_convenience(CONF_DEST_NODE, 5, 7, (uint8_t *) u32_ptr, 4);
		if (bundlemem)
			process_post(&agent_process, dtn_send_bundle_event, (void *) bundlemem);

		bundle_sent++;
		if (bundle_sent % 50 == 0)
			printf("PONG: %u\n", bundle_sent);
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
