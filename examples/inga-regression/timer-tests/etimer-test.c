/*
 * Copyright (c) 2013, TU Braunschweig
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

/* Tests order of multiple started etimer events. */

#include "contiki.h"

#include <stdio.h> /* For printf() */
#include "rtimer.h"
#include "../test.h"

#define TEST_CONF_ETIMERS 7
#define TEST_CONF_TIMES   (CLOCK_SECOND * 60), (CLOCK_SECOND / 10), (CLOCK_SECOND), (CLOCK_SECOND * 2), (CLOCK_SECOND / 2), (CLOCK_SECOND / 128), (CLOCK_SECOND * 5)
#define TEST_CONF_ORDERS  5, 1, 4, 2, 3, 6, 0

TEST_SUITE("etimer");
/*---------------------------------------------------------------------------*/
PROCESS(etimer_test_process, "etimer Test process");
AUTOSTART_PROCESSES(&etimer_test_process);
/*---------------------------------------------------------------------------*/
int running = 1;
struct etimer et[TEST_CONF_ETIMERS];
uint16_t times[TEST_CONF_ETIMERS] = {TEST_CONF_TIMES};
uint16_t evt_orders[TEST_CONF_ETIMERS] = {TEST_CONF_ORDERS};
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
PROCESS_THREAD(etimer_test_process, ev, data)
{
  PROCESS_BEGIN();

  TEST_BEGIN("etimer-test");

  static int idx;

  for(idx = 0; idx < TEST_CONF_ETIMERS; idx++) {
    etimer_set(&et[idx], times[idx]);
  }

  idx = 0;
  while(running) {

    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_TIMER);

    printf("event arrived from\n", data);
    // check if events arrive in the same order we placed them
    TEST_EQUALS((struct etimer*) data - &et[0], evt_orders[idx++]);

    if (idx == TEST_CONF_ETIMERS) {
      TESTS_DONE();
    }

  }

  TEST_END();


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

