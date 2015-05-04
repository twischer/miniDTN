/*
 * Copyright (c) 2012, Wolf-Bastian Pöttner <poettner@ibr.cs.tu-bs.de>
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
 */

/**
 * \file
 *         A test to verify the (currently selected) hash implementation
 * \author
 *         Wolf-Bastian Pöttner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "sys/test.h"
#include "sys/test.h"
#include "sys/profiling/profiling.h"
#include "watchdog.h"

#include "agent.h"
#include "hash.h"

extern const struct hash_driver hash_xxfast;
extern const struct hash_driver hash_xor;


#define LENGTH 24

/*---------------------------------------------------------------------------*/
PROCESS(test_process, "TEST");

AUTOSTART_PROCESSES(&test_process);

PROCESS_THREAD(test_process, ev, data)
{
	static uint32_t expected_result[8];
	uint8_t mytest[LENGTH];
	int i;
	int mode = 0;
	int errors = 0;
	uint32_t output_buffer;
	uint32_t output_ptr;
	uint32_t output_copy;
	static uint32_t time_start, time_stop;
	uint32_t one;
	uint32_t two;
	uint32_t three;
	uint32_t four;
	uint32_t five;
	uint32_t six;

	PROCESS_BEGIN();

	PROCESS_PAUSE();

	profiling_init();
	profiling_start();

	printf("Starting tests with %s\n", HASH.name);

	if( HASH.init == hash_xxfast.init) {
		expected_result[0] = 3795990532UL;
		expected_result[1] = 2548683827UL;
		expected_result[2] = 2791634952UL;
		expected_result[3] = 420990534UL;
		expected_result[4] = 143041184UL;
		expected_result[5] = 3992524226UL;
		expected_result[6] = 1018404474UL;
		expected_result[7] = 560019645UL;
	} else if( HASH.init == hash_xor.init ) {
		expected_result[0] = 67372036UL;
		expected_result[1] = 0UL;
		expected_result[2] = 0UL;
		expected_result[3] = 453248004UL;
		expected_result[4] = 436470788UL;
		expected_result[5] = 67372037UL;
		expected_result[6] = 67372036UL;
		expected_result[7] = 0UL;
	} else {
		printf("Expected results are unknown\n");
	}

	/* Profile initialization separately */
	profiling_stop();
	watchdog_stop();
	profiling_report("init", 0);
	watchdog_start();
	printf("Init done, starting test\n");

	profiling_init();
	profiling_start();

	for(mode=0; mode<8; mode ++) {
		if( mode == 0 ) {
			for(i=0; i<LENGTH; i++) {
				mytest[i] = i;
			}
		} else if( mode == 1 ) {
			memset(mytest, 0, LENGTH);
		} else if( mode == 2 ) {
			memset(mytest, 1, LENGTH);
		} else if( mode == 3 ) {
			for(i=0; i<LENGTH; i++) {
				mytest[i] = i;
			}
			mytest[15]++;
		} else if( mode == 4 ) {
			for(i=0; i<LENGTH; i++) {
				mytest[i] = i;
			}
			mytest[15]++;
			mytest[15]++;
		} else if( mode == 5 ) {
			for(i=0; i<LENGTH; i++) {
				mytest[i] = i;
			}
			mytest[0]++;
		} else if( mode == 6 ) {
			for(i=0; i<LENGTH; i++) {
				mytest[i] = 0xFF - i;
			}
		} else if( mode == 7 ) {
			memset(mytest, 0xFF, LENGTH);
		} else {
			printf("BÄM\n");
		}

		printf("INPUT: ");
		for(i=0; i<LENGTH; i++) {
			printf("%02X ", mytest[i]);
		}
		printf("\n");

		printf("\t Expected: %lu \n", expected_result[mode]);

		output_buffer = HASH.hash_buffer(mytest, LENGTH);
		printf("\t Buffer: %lu \n", output_buffer);

		output_ptr = HASH.hash_convenience_ptr((uint32_t *) &mytest[0], (uint32_t *) &mytest[4], (uint32_t *) &mytest[8], (uint32_t *) &mytest[12], (uint32_t *) &mytest[16], (uint32_t *) &mytest[20]);
		printf("\t Pointer: %lu \n", output_ptr);

		memcpy(&one, &mytest[0], sizeof(uint32_t));
		memcpy(&two, &mytest[4], sizeof(uint32_t));
		memcpy(&three, &mytest[8], sizeof(uint32_t));
		memcpy(&four, &mytest[12], sizeof(uint32_t));
		memcpy(&five, &mytest[16], sizeof(uint32_t));
		memcpy(&six, &mytest[20], sizeof(uint32_t));

		output_copy = HASH.hash_convenience(one, two, three, four, five, six);
		printf("\t Copy: %lu \n", output_copy);

		if( (output_buffer != output_ptr) || (output_buffer != output_copy) || (output_buffer != expected_result[mode]) ) {
			errors ++;
		}
	}

	printf("Tests done\n");

	printf("Testing performance...\n");

	time_start = test_precise_timestamp();

	for(mode=0; mode<10000; mode++) {
		one = mode;
		two = mode;
		three = mode;
		four = mode;
		five = mode;
		six = mode;

		if( mode % 100 == 0 ) {
			watchdog_periodic();
		}

		output_copy = HASH.hash_convenience(one, two, three, four, five, six);
	}

	time_stop = test_precise_timestamp();

	profiling_stop();
	watchdog_stop();
	profiling_report("hash-10000", 0);
	watchdog_start();

	TEST_REPORT("throughput", 10000*CLOCK_SECOND, time_stop-time_start, "hashes/s");

	if( errors > 0 ) {
		TEST_FAIL("hash broken");
	} else {
		TEST_PASS();
	}

	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
