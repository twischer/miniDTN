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
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
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

//#include "net/dtn/realloc.h"

#include "net/uDTN/dtn_config.h"
#include "net/uDTN/storage.h"
#include "mmem.h"
#include "watchdog.h"
#include "pressure-sensor.h"

#define SEND_INTERVAL 60  //in 5 sec

#define CONF_SEND_TO_NODE 5
#ifndef CONF_SEND_TO_NODE
#error "I need a destination node - set CONF_SEND_TO_NODE"
#endif

/*---------------------------------------------------------------------------*/
PROCESS (hello_world_process, "Hello world process");
AUTOSTART_PROCESSES (&hello_world_process);
static struct registration_api reg;
struct bundle_t bundle;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD (hello_world_process, ev, data)
{
	uint8_t i;
	static struct etimer timer;
	static uint16_t bundles_sent = 0;
	static uint32_t time_start, time_stop;
	uint16_t userdata[40];
	uint32_t tmp;

	PROCESS_BEGIN();
	agent_init();
	reg.status = 1;
	reg.application_process = &hello_world_process;
	reg.app_id = 25;
	printf ("MAIN: event= %u\n", dtn_application_registration_event);
	printf ("main app_id %lu process %p\n", reg.app_id, &agent_process);
	etimer_set (&timer,  CLOCK_SECOND * 5);
	PROCESS_WAIT_EVENT_UNTIL (etimer_expired (&timer));
	process_post (&agent_process, dtn_application_registration_event, &reg);

	SENSORS_ACTIVATE (pressure_sensor);
	etimer_set (&timer,  CLOCK_SECOND);
	PROCESS_WAIT_UNTIL (etimer_expired (&timer));

	etimer_set (&timer,  CLOCK_SECOND * 5);
	uint8_t sec5_count = 0;
	int16_t temp[SEND_INTERVAL];
	uint16_t press[SEND_INTERVAL];
	time_start = clock_seconds();
	while (1) {
		
		PROCESS_WAIT_UNTIL (etimer_expired (&timer));
		etimer_reset (&timer);

		press[sec5_count] = pressure_sensor.value (PRESS);
		temp[sec5_count] = pressure_sensor.value (TEMP);
		if (sec5_count == SEND_INTERVAL) {

			sec5_count = 0;
			uint8_t i=0;
			int32_t temp_av=0;
			uint32_t press_av=0;
			for (;i<SEND_INTERVAL;i++){
				temp_av+=temp[i];
				press_av+=press[i];
			}
			temp_av/=SEND_INTERVAL;
			press_av/=SEND_INTERVAL;
			create_bundle (&bundle);

			/* Source and destination */
			tmp = CONF_SEND_TO_NODE;
			set_attr (&bundle, DEST_NODE, &tmp);
			tmp = 25;
			set_attr (&bundle, DEST_SERV, &tmp);
			tmp = dtn_node_id;
			set_attr (&bundle, SRC_NODE, &tmp);
			set_attr (&bundle, SRC_SERV, &tmp);
			set_attr (&bundle, CUST_NODE, &tmp);
			set_attr (&bundle, CUST_SERV, &tmp);

			tmp = 0;
			set_attr (&bundle, FLAGS, &tmp);
			tmp = 1;
			set_attr (&bundle, REP_NODE, &tmp);
			set_attr (&bundle, REP_SERV, &tmp);

			/* Set the sequence number to the number of budles sent */
			tmp = bundles_sent;
			set_attr (&bundle, TIME_STAMP_SEQ_NR, &tmp);

			tmp = 2000;
			set_attr (&bundle, LIFE_TIME, &tmp);
			tmp = 4;
			set_attr (&bundle, TIME_STAMP, &tmp);

			/* Add the payload */
			userdata[0]=(int16_t)temp_av;
			userdata[1]=(uint16_t)press_av;
			add_block (&bundle, 1, 2, userdata, 4);

			process_post (&agent_process, dtn_send_bundle_event, (void *) &bundle);
		}
		sec5_count++;
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
