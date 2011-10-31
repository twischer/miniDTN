/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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

#include "contiki.h"
#include "lib/random.h"
//#include "sys/ctimer.h"
//#include "sys/etimer.h"
#include "net/uip.h"
//#include "net/uip-ds6.h"

#include "simple-udp.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234

#define SEND_INTERVAL		(20 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection broadcast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(broadcast_example_process, "UDP broadcast example process");
AUTOSTART_PROCESSES(&broadcast_example_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  static struct ctimer ct;
	static void (*light_off) = leds_off;
	static unsigned char two = LEDS_YELLOW;
	//leds_init();	

	leds_invert(LEDS_YELLOW);
	//leds_on(LEDS_YELLOW);
  printf("Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);

	//TODO warum geht hier der Timer nicht?
	//ctimer_set(&ct,  CLOCK_SECOND*2, light_off, &two);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_example_process, ev, data)
{
  static struct etimer et;
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

	//muss vor leds_init(); stehen !?
  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

	leds_init();	
	SENSORS_ACTIVATE(button_sensor);//activate button

  while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

		leds_on(LEDS_GREEN);

    printf("Sending broadcast\n");
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, "Test", 4, &addr);

		etimer_set(&et,  CLOCK_SECOND*2);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		leds_off(LEDS_GREEN);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
