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
 * $Id:$
 */

/**
 * \file
 *         A program profiling various mmem usage scenarios
 * \author
 *         Daniel Willmann <daniel@totalueberwachung.de>
 */

#include "contiki.h"
#include "lib/mmem.h"
#include "sys/profiling.h"
#include "sys/test.h"

#include <stdio.h>

#define NUM_CHUNKS 5
#define CHUNK_SIZE 100

uint32_t cycles;

/*---------------------------------------------------------------------------*/
PROCESS(mmem_inorder_tester, "MMEM free in order");
PROCESS(mmem_revorder_tester, "MMEM free reverse order");
PROCESS(profiler, "profiler");
AUTOSTART_PROCESSES(&profiler);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mmem_inorder_tester, ev, data)
{
  uint8_t i;
  static struct mmem memlist[NUM_CHUNKS];

  PROCESS_BEGIN();

  cycles = 0;
  printf("Starting MMEM inorder test\n");

  while (1) {
    /* Allocate the memory... */
    for (i=0;i<NUM_CHUNKS;i++) {
      if (!mmem_alloc(&memlist[i], CHUNK_SIZE))
	      TEST_FAIL("mmem_alloc failed\n");
    }

    /* ...and free it again */
    for (i=0;i<NUM_CHUNKS;i++) {
      mmem_free(&memlist[i]);
    }
    cycles += 1;

    PROCESS_PAUSE();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mmem_revorder_tester, ev, data)
{
  uint8_t i;
  static struct mmem memlist[NUM_CHUNKS];

  PROCESS_BEGIN();

  cycles = 0;
  printf("Starting MMEM reverse order test\n");

  while (1) {
    /* Allocate the memory... */
    for (i=0;i<NUM_CHUNKS;i++) {
      if (!mmem_alloc(&memlist[i], CHUNK_SIZE))
	      TEST_FAIL("mmem_alloc failed\n");
    }

    /* ...and free it again - this time in reverse */
    for (i=NUM_CHUNKS;i>0;i--) {
      mmem_free(&memlist[i-1]);
    }
    cycles += 1;

    PROCESS_PAUSE();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(profiler, ev, data)
{
	static struct etimer timer;
	PROCESS_BEGIN();

	mmem_init();
	profiling_init();
	profiling_start();
	process_start(&mmem_inorder_tester, NULL);
	etimer_set(&timer, CLOCK_SECOND*20);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));
	process_exit(&mmem_inorder_tester);
	profiling_stop();
	profiling_report("mmem-inorder", 0);
	TEST_REPORT("mmem-inorder", cycles*NUM_CHUNKS, 20, "mmem_alloc/s");

	/* Reset profiling for next test */
	profiling_init();
	profiling_start();
	process_start(&mmem_revorder_tester, NULL);
	etimer_set(&timer, CLOCK_SECOND*20);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));
	process_exit(&mmem_revorder_tester);
	profiling_stop();
	profiling_report("mmem-revorder", 0);
	TEST_REPORT("mmem-revorder", cycles*NUM_CHUNKS, 20, "mmem_alloc/s");
	TEST_PASS();

	PROCESS_END();
}
