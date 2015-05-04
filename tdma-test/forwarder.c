/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 *         Best-effort single-hop unicast example
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"

#include "dev/button-sensor.h"
#include "sys/profiling/profiling.h"
#include "dev/leds.h"
#include "sys/test.h"


#include "radio/rf230rt/rf230rt.h"

#include <stdio.h>


/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "Example unicast");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d,  %s\n",
	 from->u8[0], from->u8[1],(char *)packetbuf_dataptr());
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
//linkaddr_t last_node;
//int8_t last_seq;
uint8_t count = 30;
volatile uint8_t end=0;
uint8_t forward(uint8_t len, uint8_t *data)
{
//	uint8_t i=0;
//	if (    (data[5] != linkaddr_node_addr.u8[1] || data[6] != linkaddr_node_addr.u8[0]) 	//is the packet for me?
//			&& !(linkaddr_cmp(&last_node, (linkaddr_t*)&data[7]) && data[2] == last_seq)){ 	//is this packet the same as the last one forwarded?
//		linkaddr_copy(&last_node,(linkaddr_t*)&data[7]);
//		last_seq=data[2];
////		for (;i < len; i++){
////			printf("0x%02x ",data[i]);
////		}
////		printf("\n");
//		return 1;
//	}
//	return 0;
	printf("x");
	if ( !count ){
		end=1;
	}else{
		count--;
	}
	return 1;

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)
    
  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);
  /* profiling_init(); */
  /* profiling_start(); */


  while(1) {
    static struct etimer et;
    linkaddr_t addr;
    
    rf230_set_forward(&forward);
    etimer_set(&et, CLOCK_SECOND);
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    etimer_set(&et, CLOCK_SECOND);
    /* printf("radio state is %d\n", radio_get_trx_state()); */
	/* printf("send packet to 0,1\n"); */
    /* packetbuf_copyfrom("Hello", 5); */
    /* addr.u8[0] = 5; */
    /* addr.u8[1] = 0; */
    /* if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) { */
    /*   unicast_send(&uc, &addr); */
    /* } */
    if (end){
		/* profiling_stop(); */
		/* TEST_PASS(); */
		end=0;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
