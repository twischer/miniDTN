/*
 * Copyright (c) 2011, Daniel Willmann <daniel@totalueberwachung.de>
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
 * $Id:$
 */

/**
 * \file
 *         A simple LED test application demonstrating the profiling functionality
 * \author
 *         Daniel Willmann <daniel@totalueberwachung.de>
 */

#include "contiki.h"
#include "dev/leds.h"
#include "sys/profiling.h"
#include "sys/sprofiling.h"
#include "sys/test.h"

#include <stdio.h> /* For printf() */

/*---------------------------------------------------------------------------*/
PROCESS(led_test_green, "LED tester");
PROCESS(led_test_red, "LED tester");
PROCESS(profiler, "profiler");
AUTOSTART_PROCESSES(&led_test_green, &led_test_red, &profiler);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(led_test_green, ev, data)
{
  uint16_t i;
  static struct etimer timer;
  PROCESS_BEGIN();

  printf("Starting LED test (green)\n");

  while (1) {
	  leds_on(LEDS_GREEN);
	  for (i=0;i<1000;i++) {
		  asm volatile("nop");
	  }
	  leds_off(LEDS_GREEN);
	  etimer_set(&timer, CLOCK_SECOND/10);
	  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(led_test_red, ev, data)
{
  uint16_t i;
  static struct etimer timer;
  PROCESS_BEGIN();

  printf("Starting LED test (red)\n");

  while (1) {
	  leds_on(LEDS_YELLOW);
	  etimer_set(&timer, CLOCK_SECOND/10);
	  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
	  leds_off(LEDS_YELLOW);
	  etimer_set(&timer, CLOCK_SECOND/20);
	  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
	  leds_on(LEDS_YELLOW);
	  etimer_set(&timer, CLOCK_SECOND/5);
	  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
	  leds_off(LEDS_YELLOW);
	  etimer_set(&timer, CLOCK_SECOND/2);
	  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(profiler, ev, data)
{
	static struct etimer timer;
	PROCESS_BEGIN();

#ifdef STAT_PROFILE
	sprofiling_init();
	sprofiling_start();
#else
	profiling_init();
	profiling_start();
#endif

	while (1) {
		etimer_set(&timer, CLOCK_SECOND*30);
		PROCESS_WAIT_UNTIL(etimer_expired(&timer));
		etimer_set(&timer, CLOCK_SECOND*30);
		PROCESS_WAIT_UNTIL(etimer_expired(&timer));
		etimer_set(&timer, CLOCK_SECOND*30);
		PROCESS_WAIT_UNTIL(etimer_expired(&timer));
		etimer_set(&timer, CLOCK_SECOND*30);
		PROCESS_WAIT_UNTIL(etimer_expired(&timer));
#ifdef STAT_PROFILE
		sprofiling_stop();
		sprofiling_report("led-test", 0);
		TEST_PASS();
		sprofiling_start();
#else
		profiling_stop();
		profiling_report("led-test", 0);
		TEST_PASS();
		profiling_start();
#endif
	}
	PROCESS_END();
}
