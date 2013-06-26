/*
 * Copyright (c) 2012, Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
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
 *         A test to verify the (currently selected) redundancy implementation
 * \author
 *         Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "sys/test.h"
#include "sys/profiling.h"
#include "watchdog.h"

#include "net/uDTN/redundancy.h"
#include "net/uDTN/bundle.h"

/*---------------------------------------------------------------------------*/
PROCESS(test_process, "TEST");

AUTOSTART_PROCESSES(&test_process);

PROCESS_THREAD(test_process, ev, data)
{
	static uint32_t time_start = 0;
	static uint32_t time_stop = 0;
	static uint32_t errors = 0;
	static uint32_t collisions = 0;
	uint32_t i = 0;
	uint32_t test_data[10] = {251284450, 371537862, 425973621, 81975339, 170405567, 225666122, 421777121, 118094495, 410814580, 414999832};

	PROCESS_BEGIN();
	PROCESS_PAUSE();

	printf("Init done, starting test using redudancy implementation %s\n", REDUNDANCE.name);

	// Initialize the profiling
	profiling_init();
	profiling_start();

	// Measure the current time
	time_start = test_precise_timestamp();

	// -----------------------------------
	// Check if no bundles are stored
	printf("Making sure list is empty\n");
	for(i=0; i<0xFFFFF; i++) {
		if( REDUNDANCE.check(i) ) {
			errors ++;
			printf("ERROR: %lu reported to be set but should be empty\n", i);
		}

		// Keep the watchdog happy
		if( i % 10 == 0 ) {
			watchdog_periodic();
		}

		if( i % 10000 == 0 ) {
			printf("\t%lu...\n", i);
		}
	}

	// -----------------------------------
	// Store 10 bundles
	printf("Storing 10 bundles...\n");
	for(i=0; i<10; i++) {
		REDUNDANCE.set(test_data[i]);
	}

	// Keep the watchdog happy
	watchdog_periodic();

	// -----------------------------------
	// Retrieve the 10 bundles
	printf("Reading 10 bundles back...\n");
	for(i=0; i<10; i++) {
		if( !REDUNDANCE.check(test_data[i]) ) {
			printf("ERROR: %lu should be set but is not\n", i);
			errors++;
		} else {
			printf("\t%lu is fine\n", test_data[i]);
		}
	}

	// Keep the watchdog happy
	watchdog_periodic();

	// -----------------------------------
	// Check how many bundle ID are actually reported as set
	printf("Counting collisions...\n");
	for(i=0; i<0xFFFFF; i++) {
		if( REDUNDANCE.check(i) ) {
			printf("\tcollision at %lu\n", i);
			collisions ++;
		}

		// Keep the watchdog happy
		if( i % 10 == 0 ) {
			watchdog_periodic();
		}

		if( i % 10000 == 0 ) {
			printf("\t%lu...\n", i);
		}
	}

	printf("We have %lu collisions\n", collisions);

	// -----------------------------------
	// Now set 1000 bundle IDs and check the last 10 of them
	printf("Setting 1000 bundle IDs...\n");
	for(i=0; i<1000; i++) {
		REDUNDANCE.set(i);

		// Keep the watchdog happy
		if( i % 10 == 0 ) {
			watchdog_periodic();
		}
	}

	printf("Verifying IDs...\n");
	for(i=990; i<1000; i++) {
		if( !REDUNDANCE.check(i) ) {
			printf("ERROR: %lu should be set but is not\n", i);
			errors++;
		}

		// Keep the watchdog happy
		if( i % 5 == 0 ) {
			watchdog_periodic();
		}
	}

	printf("Done\n");

	// Measure the current time
	time_stop = test_precise_timestamp();

	watchdog_stop();
	profiling_report("redundancy", 0);

	TEST_REPORT("No of errors", errors, 1, "errors");
	TEST_REPORT("Duration", time_stop-time_start, CLOCK_SECOND, "s");
	TEST_REPORT("No of collisions", collisions, 1, "collisions");

	if( errors > 0 ) {
		TEST_FAIL("More than 1 error occurred");
	} else {
		TEST_PASS();
	}

	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
