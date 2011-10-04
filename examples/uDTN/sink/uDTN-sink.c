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


//#include "../platform/avr-raven/cfs-coffee-arch.h"
//#include "cfs.h"

#include "process.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/uDTN/API_registration.h"
#include "net/uDTN/API_events.h"
#include "net/uDTN/agent.h"
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/sdnv.h"
#include "etimer.h"
  #define FOO { {4, 0 } }


#include <stdio.h> /* For printf() */
  struct etimer timer;
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
  static struct registration_api reg;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Hello, world\n");
  
  agent_init();
  submit_data_to_application_event = process_alloc_event();
//  test_init();
  reg.status=1;
  reg.application_process=&hello_world_process;
  reg.app_id=25;
  printf("MAIN: event= %u\n",dtn_application_registration_event);
  printf("main app_id %lu process %p\n", reg.app_id, &agent_process);
  uint32_t foo=0;
  etimer_set(&timer,  CLOCK_SECOND*5);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  printf("foooooo\n");
  process_post(&agent_process, dtn_application_registration_event,&reg);
  static uint16_t count=0;
  static uint32_t loss=1, iold=0;
  static uint16_t nums[1000];
  uint16_t i;
  for(i=0;i<1000;i++){
  	nums[i]=0;
  }
  while (1){
  	PROCESS_WAIT_EVENT_UNTIL(ev);
	if(ev == submit_data_to_application_event) {
		leds_on(1);
		struct bundle_t *bundle;
		bundle = (struct bundle_t *) data;
//		printf("Paketinhalt: ");
//		for (i=0; i<27; i++){
//			printf("%x " ,*((uint8_t *) bundle->mem.ptr+i));
//		}

		count++;
	//	printf("%u\n",count);
		if(count > 8){
//			leds_off(1);
//			count=0;
//			printf(" %u \n",clock_time());
		}
		if (count == 1){
//			leds_on(1);
		}

		uint32_t j;
		sdnv_decode(bundle->mem.ptr+bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET],bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE],&j);
		if(j==iold){
			loss+= (j-iold);
		}
		//printf("rec: %u %lu %lu \n",count,j, loss-1);
		nums[j]=1;
//		if (j==998||j==999){
	//	}
			
		iold=j+1;
		if (count==1){
			printf("go\n");
		}
		if (count==1000){
			leds_off(1);
			printf("done\n");
			uint16_t l=0;
			for(i=0;i<j+1;i++){
				watchdog_periodic();
				if(nums[i]!=1){
					l++;
				}
			}
			printf("loss: %u\n",l);
		}
		delete_bundle(bundle);
	}
//	cc2420_read(packetbuf_dataptr(),128);
  }
#if 0
	/*#define PRINTF printf
	PRINTF("Formatting FLASH file system for coffee...");
	cfs_coffee_format();
	PRINTF("Done!\n");
	int fa = cfs_open( "/index.html", CFS_WRITE);
	int r = cfs_write(fa, &"It works!", 9);
	if (r<0) PRINTF("Can''t create /index.html!\n");
	cfs_close(fa);
*/
//	int foo=coffee_file_test();
//	printf(" FAIL %d\n",foo);
#endif
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
