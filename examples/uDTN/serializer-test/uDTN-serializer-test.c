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

#define TEST_BUNDLES 10

uint8_t static_compare_bundle[] = {
		0x06, 0x10, 0x0D, 0x4D, 0x58, 0x63, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x9C, 0x10, 0x00, 0x01, 0x08, 0x3C, 0x00,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
		0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
		0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B
};
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

	// Set VERSION
	uint32_t version = 6;
	bundle_set_attr(ptr, VERSION, &version);

	// Set FLAGS
	uint32_t flags = BUNDLE_FLAG_SINGLETON;
	bundle_set_attr(ptr, FLAGS, &flags);

	// Set DEST_NODE
	uint32_t dest_node = 77;
	bundle_set_attr(ptr, DEST_NODE, &dest_node);

	// Set DEST_SERV
	uint32_t dest_serv = 88;
	bundle_set_attr(ptr, DEST_SERV, &dest_serv);

	// Set SRC_NODE
	uint32_t src_node = 99;
	bundle_set_attr(ptr, SRC_NODE, &src_node);

	// Set SRC_SERV
	uint32_t src_serv = 110;
	bundle_set_attr(ptr, SRC_SERV, &src_serv);

	// Set TIME_STAMP
	uint32_t timestamp = 7;
	bundle_set_attr(ptr, TIME_STAMP, &timestamp);

	// Set TIME_STAMP_SEQ_NR
	bundle_set_attr(ptr, TIME_STAMP_SEQ_NR, &sequence_number);

	// Set LIFE_TIME
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

void hexdump(uint8_t * buffer, uint8_t length) {
	int i;

	for(i=0; i<length; i++) {
		printf("%02X ", buffer[i]);
		if( (i+1) % 20 == 0 ) {
			printf("\n");
		}
	}
	printf("\n");

}
uint8_t my_static_compare(uint8_t * buffer, uint8_t length) {
	int mylength = 0;
	int errors = 0;
	int i;

	mylength = sizeof(static_compare_bundle);
	if( length < mylength ) {
		mylength = length;
		errors ++;
	}

	for(i=0; i<mylength; i++) {
		if( buffer[i] != static_compare_bundle[i] ) {
			printf("%u mismatch %02X vs. %02X\n", i, buffer[i], static_compare_bundle[i]);
			errors ++;
		}
	}

	if( errors ) {
		printf("Static: \n");
		hexdump(static_compare_bundle, sizeof(static_compare_bundle));
		printf("\nSerialized:\n");
		hexdump(buffer, length);
	}
	return errors;
}

uint8_t my_compare_bundles(struct mmem * original, struct mmem * compare) {
	uint32_t attribute_original;
	uint32_t attribute_compare;
	uint8_t errors = 0;
	int i;

	for(i=VERSION; i<=FRAG_OFFSET; i++) {
		attribute_original = 0;
		attribute_compare = 0;

		bundle_get_attr(original, i, &attribute_original);
		bundle_get_attr(compare, i, &attribute_compare);

		if( attribute_original != attribute_compare ) {
			printf("Attribute %u mismatch: %lu vs. %lu\n", i, attribute_original, attribute_compare);
			errors ++;
		}
	}

	return errors;
}

PROCESS_THREAD(test_process, ev, data)
{
	static int n;
	static int i;
	static int errors = 0;
	static struct etimer timer;
	static uint32_t time_start, time_stop;
	uint8_t buffer[128];
	int bundle_length;
	struct mmem * bundle_original = NULL;
	struct mmem * bundle_restored = NULL;
	struct mmem * bundle_spare = NULL;
	uint32_t bundle_number;
	uint32_t bundle_number_spare;

	PROCESS_BEGIN();

	PROCESS_PAUSE();

	profiling_init();
	profiling_start();

	// Wait again
	etimer_set(&timer, CLOCK_SECOND);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));

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

	for(i=0; i<=1; i++) {
		struct mmem bla;
		if( i > 0 ) {
			mmem_alloc(&bla, 1);
		}

		printf("Serializing and deserializing bundle...\n");
		if( my_create_bundle(0, &bundle_number, 3600) ) {
			printf("\tBundle created successfully \n");
		} else {
			printf("\tBundle could not be created \n");
			errors ++;
		}

		printf("Serializing and deserializing bundle...\n");
		if( my_create_bundle(1, &bundle_number_spare, 3600) ) {
			printf("\tSpare Bundle created successfully \n");
		} else {
			printf("\tSpare Bundle could not be created \n");
			errors ++;
		}

		bundle_original = BUNDLE_STORAGE.read_bundle(bundle_number);
		if( bundle_original == NULL ) {
			printf("VERIFY: MMEM ptr is invalid\n");
			errors ++;
		}

		bundle_spare = BUNDLE_STORAGE.read_bundle(bundle_number_spare);
		if( bundle_spare == NULL ) {
			printf("VERIFY: MMEM ptr is invalid\n");
			errors ++;
		}

		// Serialize the bundle
		memset(buffer, 0, 128);
		bundle_length = bundle_encode_bundle(bundle_original, buffer, 128);
		if( bundle_length < 0 ) {
			printf("SERIALIZE: fail\n");
			errors ++;
		}

		n = my_static_compare(buffer, bundle_length);
		if( n > 0 ) {
			printf("COMPARE: fail\n");
			errors += n;
		}

		// Deserialize it
		bundle_restored = bundle_recover_bundle(buffer, bundle_length);
		if( bundle_restored == NULL ) {
			printf("DESERIALIZE: unable to recover\n");
			errors ++;
		}

		n = my_compare_bundles(bundle_original, bundle_restored);
		if( n == 0 ) {
			printf("\tBundle serialized and deserialized successfully\n");
		} else {
			printf("COMPARE: differences\n");
			errors ++;
		}

		// Dellocate memory
		bundle_decrement(bundle_restored);
		bundle_restored = NULL;

		bundle_decrement(bundle_original);
		bundle_original = NULL;

		bundle_decrement(bundle_spare);
		bundle_spare = NULL;

		memset(buffer, 0, 128);

		// Delete bundle from storage
		n = BUNDLE_STORAGE.del_bundle(bundle_number, REASON_DELIVERED);
		if( n ) {
			printf("\tBundle deleted successfully\n");
		} else {
			printf("\tBundle could not be deleted\n");
			errors++;
		}


		printf("Comparing static bundle...\n");
		if( my_create_bundle(0, &bundle_number, 3600) ) {
			printf("\tReference Bundle created successfully \n");
		} else {
			printf("\ttReference Bundle could not be created \n");
			errors ++;
		}

		bundle_original = BUNDLE_STORAGE.read_bundle(bundle_number);
		if( bundle_original == NULL ) {
			printf("VERIFY: MMEM ptr is invalid\n");
			errors ++;
		}

		// Deserialize it
		bundle_restored = bundle_recover_bundle(static_compare_bundle, sizeof(static_compare_bundle));
		if( bundle_restored == NULL ) {
			printf("DESERIALIZE: unable to recover static bundle\n");
			errors ++;
		}

		// Deserialize it one more time
		bundle_spare = bundle_recover_bundle(static_compare_bundle, sizeof(static_compare_bundle));
		if( bundle_spare == NULL ) {
			printf("DESERIALIZE: unable to recover static bundle\n");
			errors ++;
		}

		n = my_compare_bundles(bundle_original, bundle_restored);
		if( n == 0 ) {
			printf("\tStatic Bundle verified successfully\n");
		} else {
			printf("COMPARE: differences\n");
			errors ++;
		}

		n = my_compare_bundles(bundle_original, bundle_spare);
		if( n == 0 ) {
			printf("\tStatic Bundle verified successfully\n");
		} else {
			printf("COMPARE: differences\n");
			errors ++;
		}

		// Dellocate memory
		bundle_decrement(bundle_restored);
		bundle_restored = NULL;

		bundle_decrement(bundle_original);
		bundle_original = NULL;

		bundle_decrement(bundle_spare);
		bundle_spare = NULL;
	}

	time_stop = test_precise_timestamp(NULL);

	watchdog_stop();
	profiling_report("serializer", 0);

	TEST_REPORT("No of errors", errors, 1, "errors");
	TEST_REPORT("Duration", time_stop-time_start, CLOCK_SECOND, "s");

	if( errors > 0 ) {
		TEST_FAIL("More than 0 errors occured");
	} else {
		TEST_PASS();
	}

	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
