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
 *         A test to verify the (currently selected) storage implementation
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

// defined in mmem.c, no function to access it though
extern unsigned int avail_memory;

uint32_t bundle_numbers[TEST_BUNDLES * 3];

unsigned int initial_memory;
uint16_t bundle_slots_free;

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
	static int mode;
	static struct etimer timer;
	static struct storage_entry_t * list_entry = NULL;
	static int ok = 0;
	static uint32_t time_start, time_stop;

	PROCESS_BEGIN();

  PROCESS_PAUSE();

	profiling_init();
	profiling_start();

	// Wait again
	etimer_set(&timer, CLOCK_SECOND);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));

	// Store the amount of free memory
	initial_memory = avail_memory;
	bundle_slots_free = BUNDLE_STORAGE.free_space(NULL);

	/* Profile initialization separately */
	profiling_stop();
	watchdog_stop();
	profiling_report("init", 0);
	watchdog_start();
	printf("Init done, starting test using %s storage\n", BUNDLE_STORAGE.name);

	profiling_init();
	profiling_start();

	// Measure the current time
	time_start = test_precise_timestamp(NULL);

	printf("Create, Read and Delete in sequence\n");
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

		n = BUNDLE_STORAGE.del_bundle(bundle_numbers[i], REASON_DELIVERED);

		if( n ) {
			printf("\tBundle %lu deleted successfully\n", i);
		} else {
			printf("\tBundle %lu could not be deleted\n", i);
			errors++;
		}

		if( BUNDLE_STORAGE.get_bundles() != NULL ) {
			printf("Bundle list is not empty\n");
			errors ++;
		}

		if( BUNDLE_STORAGE.get_bundle_num() > 0 ) {
			printf("Storage reports more than 0 bundles\n");
			errors ++;
		}

		if( BUNDLE_STORAGE.free_space(NULL) != bundle_slots_free ) {
			printf("Storage does not report enough free slots\n");
			errors ++;
		}

		if( avail_memory != initial_memory ) {
			printf("MMEM fail\n");
			errors++;
		}
	}

	for(mode=0; mode<6; mode ++) {
		PROCESS_PAUSE();

		printf("MODE %u\n", mode);

		if( BUNDLE_STORAGE.get_bundles() != NULL ) {
			printf("Bundle list is not empty\n");
			errors ++;
		}

		printf("Creating bundles...\n");
		// Create TEST_BUNDLES bundles
		for(i=0; i<TEST_BUNDLES; i++) {
			PROCESS_PAUSE();

			if( BUNDLE_STORAGE.get_bundle_num() != i ) {
				printf("Storage erroneously reports %u bundles\n", BUNDLE_STORAGE.get_bundle_num());
				errors ++;
			}

			if( (bundle_slots_free - BUNDLE_STORAGE.free_space(NULL)) != i ) {
				printf("Storage erroneously reports %u bundle slots left\n", BUNDLE_STORAGE.free_space(NULL));
				errors ++;
			}

			if( my_create_bundle(i, &bundle_numbers[i], 3600) ) {
				printf("\tBundle %lu created successfully \n", i);
			} else {
				printf("\tBundle %lu could not be created \n", i);
				errors ++;
			}

			if( (bundle_slots_free - i - 1) != BUNDLE_STORAGE.free_space(NULL) ) {
				printf("Storage erroneously reports %u bundle slots left\n", BUNDLE_STORAGE.free_space(NULL));
				errors ++;
			}

			if( BUNDLE_STORAGE.get_bundle_num() != i+1 ) {
				printf("Storage erroneously reports %u bundles\n", BUNDLE_STORAGE.get_bundle_num());
				errors ++;
			}
		}

		if( mode == 0 || mode == 1 ) {
			printf("Reading bundles back FORWARD...\n");
			// Verify the created bundles by reading them back and checking the content
			for(i=0; i<TEST_BUNDLES; i++) {
				PROCESS_PAUSE();

				if( my_verify_bundle(bundle_numbers[i], i) ) {
					printf("\tBundle %lu read back successfully \n", i);
				} else {
					printf("\tBundle %lu could not be read back and verified \n", i);
					errors ++;
				}
			}
		} else if( mode == 2 || mode == 3 ) {
			printf("Reading bundles back BACKWARD...\n");
			// Verify the created bundles by reading them back and checking the content
			for(i=TEST_BUNDLES; i>0; i--) {
				PROCESS_PAUSE();

				if( my_verify_bundle(bundle_numbers[i-1], i-1) ) {
					printf("\tBundle %lu read back successfully \n", i-1);
				} else {
					printf("\tBundle %lu could not be read back and verified \n", i-1);
					errors ++;
				}
			}
		} else if( mode == 4 ) {
			printf("Reading bundles and deleting them FORWARD...\n");
			// Verify the created bundles by reading them back and checking the content
			for(i=0; i<TEST_BUNDLES; i++) {
				PROCESS_PAUSE();

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
		} else if( mode == 5 ) {
			printf("Reading bundles and deleting them BACKWARD...\n");
			// Verify the created bundles by reading them back and checking the content
			for(i=TEST_BUNDLES; i>0; i--) {
				PROCESS_PAUSE();

				if( my_verify_bundle(bundle_numbers[i-1], i-1) ) {
					printf("\tBundle %lu read back successfully \n", i-1);
				} else {
					printf("\tBundle %lu could not be read back and verified \n", i-1);
					errors ++;
				}

				n = BUNDLE_STORAGE.del_bundle(bundle_numbers[i-1], REASON_DELIVERED);

				if( n ) {
					printf("\tBundle %lu deleted successfully\n", i-1);
				} else {
					printf("\tBundle %lu could not be deleted\n", i-1);
					errors++;
				}
			}
		}

		if( mode != 4 && mode != 5 ) {
			for( list_entry = BUNDLE_STORAGE.get_bundles();
				 list_entry != NULL;
				 list_entry = list_item_next(list_entry) ) {

				ok = 0;
				for(i=0; i<TEST_BUNDLES; i++) {
					if( list_entry->bundle_num == bundle_numbers[i] ) {
						ok = 1;
					}
				}

				if( !ok ) {
					printf("Bundle list contains bundle that should not exist\n");
					errors++;
				}
			}

			for(i=0; i<TEST_BUNDLES; i++) {
				ok = 0;

				for( list_entry = BUNDLE_STORAGE.get_bundles();
					 list_entry != NULL;
					 list_entry = list_item_next(list_entry) ) {
					if( list_entry->bundle_num == bundle_numbers[i] ) {
						ok = 1;
					}
				}

				if( !ok ) {
					printf("Bundle list does not contain bundle %lu\n", bundle_numbers[i]);
					errors ++;
				}
			}
		}

		if( mode == 0 || mode == 2 ) {
			printf("Deleting bundles FORWARD...\n");
			// Now delete all the bundles again
			for(i=0; i<TEST_BUNDLES; i++) {
				PROCESS_PAUSE();

				if( BUNDLE_STORAGE.get_bundle_num() != (TEST_BUNDLES - i) ) {
					printf("Storage erroneously reports %u bundles\n", BUNDLE_STORAGE.get_bundle_num());
					errors ++;
				}

				if( (bundle_slots_free - BUNDLE_STORAGE.free_space(NULL)) != (TEST_BUNDLES - i) ) {
					printf("Storage erroneously reports %u bundle slots left\n", BUNDLE_STORAGE.free_space(NULL));
					errors ++;
				}

				n = BUNDLE_STORAGE.del_bundle(bundle_numbers[i], REASON_DELIVERED);

				if( n ) {
					printf("\tBundle %lu deleted successfully\n", i);
				} else {
					printf("\tBundle %lu could not be deleted\n", i);
					errors++;
				}

				if( BUNDLE_STORAGE.get_bundle_num() != (TEST_BUNDLES - i - 1) ) {
					printf("Storage erroneously reports %u bundles\n", BUNDLE_STORAGE.get_bundle_num());
					errors ++;
				}
			}
		} else if( mode == 1 || mode == 3 ) {
			printf("Deleting bundles BACKWARD...\n");
			// Now delete all the bundles again
			for(i=TEST_BUNDLES; i>0; i--) {
				PROCESS_PAUSE();

				n = BUNDLE_STORAGE.del_bundle(bundle_numbers[i-1], REASON_DELIVERED);

				if( n ) {
					printf("\tBundle %lu deleted successfully\n", i-1);
				} else {
					printf("\tBundle %lu could not be deleted\n", i-1);
					errors++;
				}
			}
		}
	}

	time_stop = test_precise_timestamp(NULL);

	if( BUNDLE_STORAGE.get_bundles() != NULL ) {
		printf("Bundle list is not empty\n");
		errors ++;
	}

	printf("Verifying bundle expiration...\n");
	// Now verify the lifetime expiration of the storage
	for(i=0; i<TEST_BUNDLES; i++) {
		PROCESS_PAUSE();

		if( my_create_bundle(i, &bundle_numbers[i], 5) ) {
			printf("\tBundle %lu created successfully \n", i);
		} else {
			printf("\tBundle %lu could not be created \n", i);
			errors ++;
		}
	}

	printf("Waiting 60s to expire bundles...\n");
	// Wait 60s until all bundles must have timed out
	etimer_set(&timer, CLOCK_SECOND * 60);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));

	printf("Checking for leftovers...\n");
	// And check!
	if( BUNDLE_STORAGE.get_bundles() != NULL ) {
		printf("Bundle list is not empty\n");
		errors ++;
	}

	if( BUNDLE_STORAGE.get_bundle_num() > 0 ) {
		printf("Storage reports more than 0 bundles\n");
		errors ++;
	}

	if( BUNDLE_STORAGE.free_space(NULL) != bundle_slots_free ) {
		printf("Storage does not report enough free slots\n");
		errors ++;
	}

	if( avail_memory != initial_memory ) {
		printf("MMEM fail\n");
		errors++;
	}

	printf("Verifying that storage overwrites old bundles when new come in...\n");
	for(i=0; i<TEST_BUNDLES*2; i++) {
		PROCESS_PAUSE();

		if( my_create_bundle(i, &bundle_numbers[i], 3600) ) {
			printf("\tBundle %lu created successfully \n", i);
		} else {
			printf("\tBundle %lu could not be created \n", i);
			errors ++;
		}
	}

	PROCESS_PAUSE();

	printf("Reading back and verifying...\n");
	for(i=TEST_BUNDLES; i<TEST_BUNDLES*2; i++) {
		PROCESS_PAUSE();

		if( my_verify_bundle(bundle_numbers[i], i) ) {
			printf("\tBundle %lu read back successfully \n", i);
		} else {
			printf("\tBundle %lu could not be read back and verified \n", i);
			errors ++;
		}
	}

	printf("Checking all bundles...\n");
	for(i=TEST_BUNDLES; i<TEST_BUNDLES*2; i++) {
		ok = 0;

		for( list_entry = BUNDLE_STORAGE.get_bundles();
			 list_entry != NULL;
			 list_entry = list_item_next(list_entry) ) {
			if( list_entry->bundle_num == bundle_numbers[i] ) {
				ok = 1;
			}
		}

		if( !ok ) {
			printf("Bundle list does not contain bundle %lu\n", bundle_numbers[i]);
			errors ++;
		}
	}

	printf("Deleting...\n");
	for(i=TEST_BUNDLES; i<TEST_BUNDLES*2; i++) {
		PROCESS_PAUSE();

		if( BUNDLE_STORAGE.get_bundle_num() != ((TEST_BUNDLES*2) - i) ) {
			printf("Storage erroneously reports %u bundles, expected %u\n", BUNDLE_STORAGE.get_bundle_num(), ((TEST_BUNDLES*2) - i));
			errors ++;
		}

		if( (bundle_slots_free - BUNDLE_STORAGE.free_space(NULL)) != ((TEST_BUNDLES*2) - i) ) {
			printf("Storage erroneously reports %u bundle slots left\n", BUNDLE_STORAGE.free_space(NULL));
			errors ++;
		}

		n = BUNDLE_STORAGE.del_bundle(bundle_numbers[i], REASON_DELIVERED);

		if( n ) {
			printf("\tBundle %lu deleted successfully\n", i);
		} else {
			printf("\tBundle %lu could not be deleted\n", i);
			errors++;
		}

		if( BUNDLE_STORAGE.get_bundle_num() != ((TEST_BUNDLES*2) - i - 1) ) {
			printf("Storage erroneously reports %u bundles\n", BUNDLE_STORAGE.get_bundle_num());
			errors ++;
		}
	}

	// And check!
	if( BUNDLE_STORAGE.get_bundles() != NULL ) {
		printf("Bundle list is not empty\n");
		errors ++;
	}

	if( BUNDLE_STORAGE.get_bundle_num() > 0 ) {
		printf("Storage reports more than 0 bundles\n");
		errors ++;
	}

	if( BUNDLE_STORAGE.free_space(NULL) != bundle_slots_free ) {
		printf("Storage does not report enough free slots\n");
		errors ++;
	}

	if( avail_memory != initial_memory ) {
		printf("MMEM fail\n");
		errors++;
	}

	watchdog_stop();
	profiling_report("storage", 0);

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
