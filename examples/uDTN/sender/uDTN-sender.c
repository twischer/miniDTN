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

#include "contiki.h"

#include "net/netstack.h"
#include "net/packetbuf.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/API_events.h"
#include "net/uDTN/API_registration.h"

#include <string.h>
//#include "net/dtn/realloc.h"

#include "dev/button-sensor.h"
#include "net/uDTN/dtn_config.h"
#include "net/uDTN/storage.h"
#include "mmem.h"

#include <stdio.h> /* For printf() */
#include <stdlib.h>
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
static struct registration_api reg;
/*---------------------------------------------------------------------------*/
static struct etimer timer;
struct bundle_t bundle;
static uint16_t last_trans=0;
PROCESS_THREAD(hello_world_process, ev, data)
{
	PROCESS_BEGIN();
	
	SENSORS_ACTIVATE(button_sensor);
	agent_init();
	//test_init();
	uint8_t c=0;
	submit_data_to_application_event = process_alloc_event();
	reg.status=1;
	reg.application_process=&hello_world_process;
	reg.app_id=25;
	printf("MAIN: event= %u\n",dtn_application_registration_event);
	printf("main app_id %lu process %p\n", reg.app_id, &agent_process);
	etimer_set(&timer,  CLOCK_SECOND*5);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
	printf("foooooo\n");
	process_post(&agent_process, dtn_application_registration_event,&reg);
	static uint16_t	rec=0;
//	if((ev == sensors_event && data == &button_sensor)){
//		etimer_set(&timer, CLOCK_SECOND*0.1);
//	}
	etimer_set(&timer,  CLOCK_SECOND*0.05);
	while(1) {
		PROCESS_YIELD();
/*		if(ev == submit_data_to_application_event) {
			printf("rtt:%u\n",clock_time()-last_trans);
			struct bundle_t *bun;
			bun = (struct bundle_t *) data;
			delete_bundle(bun);
			rec=1;
		}
*/		if(etimer_expired(&timer) || (ev == sensors_event && data == &button_sensor)) {
		//if((ev == sensors_event && data == &button_sensor)) {
			rec++;
	//		j++;
			//printf("Hello, world\n");
			create_bundle(&bundle);
			uint8_t i;
			uint32_t bla=4;
		//			rimeaddr_t dest={{3,0}};
		//#if 0
			bla=1;
			set_attr(&bundle, DEST_NODE, &bla);
			bla=25;
			set_attr(&bundle, DEST_SERV, &bla);
			bla=dtn_node_id;
			set_attr(&bundle, SRC_NODE, &bla);
			set_attr(&bundle, SRC_SERV,&bla);
			set_attr(&bundle, CUST_NODE, &bla);
			set_attr(&bundle, CUST_SERV, &bla);
			bla=0;
			set_attr(&bundle, FLAGS, &bla);
			bla=1;
			set_attr(&bundle, REP_NODE, &bla);
			set_attr(&bundle, REP_SERV, &bla);
			set_attr(&bundle, TIME_STAMP_SEQ_NR, &bla);
			bla=2000;
			set_attr(&bundle, LIFE_TIME, &bla);
			bla=4;
			set_attr(&bundle, TIME_STAMP, &bla);
			uint8_t foo[80]={10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};
			add_block(&bundle, 1,2,foo,80);
			
//			printf("main size: %u\n",bundle.size);
			uint8_t *tmp=(uint8_t *) bundle.mem.ptr;
			for(i=0; i<bundle.size; i++){
				//printf("%x ",*tmp);
				tmp++;

			}
//		printf("\n");
			process_post(&agent_process,dtn_send_bundle_event,(void *) &bundle);
			last_trans=clock_time();
//			if (BUNDLE_STORAGE.get_bundle_num() <39){
//			if (rec <1000){
				//etimer_reset(&timer);
				etimer_set(&timer, CLOCK_SECOND*0.01);
//			}

//			}else{
//				etimer_set(&timer, CLOCK_SECOND*20);
//			}
			


		continue;			
		}
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
