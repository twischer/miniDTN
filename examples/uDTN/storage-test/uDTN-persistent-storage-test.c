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
 *         A test to verify the persistence of the storage implementation
 * \author
 *         Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "sys/test.h"
#include "sys/test.h"
#include "sys/profiling.h"
#include "watchdog.h"
#include "list.h"
#include "cfs-coffee.h"

#include "net/uDTN/agent.h"
#include "net/uDTN/bundle.h"
#include "net/uDTN/storage.h"

#define TEST_BUNDLES BUNDLE_STORAGE_SIZE

uint32_t bundle_numbers[TEST_BUNDLES * 3];

/*---------------------------------------------------------------------------*/
PROCESS(test_process, "TEST");

AUTOSTART_PROCESSES(&test_process);

uint8_t my_create_bundle(uint32_t sequence_number, uint32_t * bundle_number, uint32_t lifetime) {
	struct mmem * ptr = NULL;
	int n;
	uint32_t i;
	uint8_t payload[60];
	uint32_t * bundle_number_ptr;

	ptr = bundle_create_bundle();
	if( ptr == NULL ) {
		printf("CREATE: Bundle %lu could not be allocated\n", sequence_number);
		return 0;
	}

	// Set all attributes
	for(i=VERSION; i<=FRAG_OFFSET; i++) {
		bundle_set_attr(ptr, i, &i);
	}

	// But set the sequence number to something monotonically increasing
	bundle_set_attr(ptr, TIME_STAMP_SEQ_NR, &sequence_number);

	// Set the lifetime
	bundle_set_attr(ptr, LIFE_TIME, &lifetime);

	// Fill the payload
	for(i=0; i<60; i++) {
		payload[i] = i + (uint8_t) sequence_number;
	}

	// Add a payload block
	bundle_add_block(ptr, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload, 60);

	// And tell storage to save the bundle
	n = BUNDLE_STORAGE.save_bundle(ptr, &bundle_number_ptr);
	if( !n ) {
		printf("CREATE: Bundle %lu could not be created\n", sequence_number);
		return 0;
	}

	// Copy over the bundle number
	*bundle_number = *bundle_number_ptr;

	return 1;
}

uint8_t my_verify_bundle(uint32_t bundle_number, uint32_t sequence_number) {
	uint32_t attr = 0;
	uint32_t i;
	int errors = 0;
	struct bundle_block_t * block = NULL;
	struct mmem * ptr = NULL;

	ptr = BUNDLE_STORAGE.read_bundle(bundle_number);
	if( ptr == NULL ) {
		printf("VERIFY: MMEM ptr is invalid\n");
		return 0;
	}

	struct bundle_t * bundle = (struct bundle_t *) MMEM_PTR(ptr);
	if( bundle == NULL ) {
		printf("VERIFY: bundle ptr is invalid\n");
		return 0;
	}

	// Verify the attributes
	for(i=VERSION; i<=FRAG_OFFSET; i++) {
		bundle_get_attr(ptr, i, &attr);

		if( i == TIME_STAMP_SEQ_NR ||
			i == LENGTH ||
			i == DIRECTORY_LEN ||
			i == LIFE_TIME ) {
			continue;
		}

		if( attr != i ) {
			printf("VERIFY: attribute %lu mismatch\n", i);
			errors ++;
		}
	}

	// Verify the sequence number
	bundle_get_attr(ptr, TIME_STAMP_SEQ_NR, &attr);
	if( attr != sequence_number ) {
		printf("VERIFY: sequence number mismatch\n");
		errors ++;
	}

	block = bundle_get_payload_block(ptr);
	if( block == NULL ) {
		printf("VERIFY: unable to get payload block\n");
		errors ++;
	} else {
		// Fill the payload
		for(i=0; i<60; i++) {
			if( block->payload[i] != (i + (uint8_t) sequence_number) ) {
				printf("VERIFY: payload byte %lu mismatch\n", i);
				errors ++;
			}
		}
	}

	bundle_decrement(ptr);

	if( errors ) {
		return 0;
	}

	return 1;
}

PROCESS_THREAD(test_process, ev, data)
{
	static int n;
	static uint32_t i;
	static int errors = 0;
	static struct etimer timer;
	static uint32_t time_start, time_stop;

	PROCESS_BEGIN();

	/* Initialize the flash before the storage comes along */
	printf("Intializing Flash...\n");
	cfs_coffee_format();

	PROCESS_PAUSE();

	profiling_init();
	profiling_start();

	// Wait again
	etimer_set(&timer, CLOCK_SECOND);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));

	printf("Init done, starting test using %s storage\n", BUNDLE_STORAGE.name);

	profiling_init();
	profiling_start();

	// Measure the current time
	time_start = test_precise_timestamp(NULL);

	printf("Create and Verify bundles in sequence\n");
	for(i=0; i<TEST_BUNDLES; i++) {
		PROCESS_PAUSE();

		if( my_create_bundle(i, &bundle_numbers[i], 3600) ) {
			printf("\tBundle %lu created successfully \n", i);
		} else {
			printf("\tBundle %lu could not be created \n", i);
			errors ++;
			continue;
		}

		if( my_verify_bundle(bundle_numbers[i], i) ) {
			printf("\tBundle %lu read back successfully \n", i);
		} else {
			printf("\tBundle %lu could not be read back and verified \n", i);
			errors ++;
		}
	}

	printf("Reinitialize storage\n");
	/* Reinitialize the storage and see, if the bundles persist */
	BUNDLE_STORAGE.init();

	printf("Verify and Delete bundles in sequence\n");
	for(i=0; i<TEST_BUNDLES; i++) {
		if( my_verify_bundle(bundle_numbers[i], i) ) {
			printf("\tBundle %lu read back successfully \n", i);
		} else {
			printf("\tBundle %lu could not be read back and verified \n", i);
			errors ++;
		}

		n = BUNDLE_STORAGE.del_bundle(bundle_numbers[i], REASON_DELIVERED);

		if( n ) {
			printf("\tBundle %lu deleted successfully\n", i);
		} else {
			printf("\tBundle %lu could not be deleted\n", i);
			errors++;
		}
	}

	time_stop = test_precise_timestamp(NULL);

	watchdog_stop();
	profiling_report("persistent-storage", 0);

	TEST_REPORT("No of errors", errors, 1, "errors");
	TEST_REPORT("Duration", time_stop-time_start, CLOCK_SECOND, "s");

	if( errors > 0 ) {
		TEST_FAIL("More than 1 error occured");
	} else {
		TEST_PASS();
	}

	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
