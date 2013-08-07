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
 */

#include "contiki.h"

#include <stdio.h> /* For printf() */
#include "rtimer.h"
#include "../test.h"

TEST_SUITE("rtimer");
/*---------------------------------------------------------------------------*/
PROCESS(rtimer_test_process, "RTimer Test process");
AUTOSTART_PROCESSES(&rtimer_test_process);
/*---------------------------------------------------------------------------*/
int running = 1;
struct etimer et;
struct rtimer rt;
static uint16_t cur_time, after_rtimer;
static int ret;	
static uint8_t done;

void rtime_call(struct rtimer *t, void *ptr){
  after_rtimer=RTIMER_NOW();
  if (after_rtimer - cur_time ==  1000 + 1){
    ret = 0;
  }else{
    ret = 1;
  }
  done = 2;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rtimer_test_process, ev, data)
{
  PROCESS_BEGIN();

  TEST_BEGIN("rtimer-test");

  etimer_set(&et, CLOCK_SECOND * 5);
  PROCESS_YIELD_UNTIL(etimer_expired(&et));

  while(running) {

    if(done == 0){
      printf("Starting test...\n");
      cur_time=RTIMER_NOW();
      rtimer_set(&rt, cur_time + 1000, 0, rtime_call,NULL);
    }

    if(done == 2){

      running = 0;
      TEST_REPORT("timediff = ", after_rtimer - cur_time, 1, "ms");

      TEST_EQUALS(ret, 0);
    }

    etimer_set(&et, CLOCK_SECOND * 1);
    PROCESS_YIELD_UNTIL(etimer_expired(&et));
  }

  TEST_END();

  TESTS_DONE();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
