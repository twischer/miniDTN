 /**
 * \addtogroup statistics Statistics module
 *
 * @{
 */

/**
 * \file uDTN Statistics Module
 * \author Wolf-Bastian Pšttner (poettner@ibr.cs.tu-bs.de)
 */

#include <string.h> // for memcpy

#include "statistics.h"
#include "dtn_config.h"
#include "agent.h"
#include "contiki.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint16_t statistics_interval = 0;
unsigned long statistics_timestamp = 0;
struct statistics_element_t statistics_array[STATISTICS_ELEMENTS];

/**
 * \brief Internal function to find out, into which array slot the information is written
 */
uint8_t statistics_get_pointer() {
	if( statistics_interval < 1 ) {
		return 0;
	}

	// Calculate since when we are recording statistics
	unsigned long elapsed = clock_seconds() - statistics_timestamp;

	// Find out in which "slot" of the array we are currently writing
	uint8_t ptr = elapsed / statistics_interval;

	if( ptr >= STATISTICS_ELEMENTS ) {
		ptr = STATISTICS_ELEMENTS - 1;
	}

	PRINTF("STATISTICS: ptr %u\n", ptr);

	return ptr;
}

/**
 * \brief Init function to be called by application
 * \return Seconds after which the statistics have to be sent off
 */
uint16_t statistics_setup(uint16_t interval)
{
	PRINTF("STATISTICS: setup(%u)\n", interval);

	// Copy config paramters
	statistics_interval = interval;

	// Reset the results (just to be sure)
	statistics_reset();

	return interval * STATISTICS_ELEMENTS;
}

/**
 * \brief Copy the statistics bundle into the provided buffer
 * \return Length of the payload
 */
uint8_t statistics_get_bundle(uint8_t * buffer, uint8_t maximum_length)
{
	if( statistics_interval < 1 ) {
		return 0;
	}

	int offset = 0;

	PRINTF("STATISTICS: get_bundle(%p, %u)\n", buffer, maximum_length);

	// Store the timestamp of this period
	memcpy(buffer + offset, &statistics_timestamp, sizeof(statistics_timestamp));
	offset += sizeof(statistics_timestamp);

	// Store how long each step is
	memcpy(buffer + offset, &statistics_interval, sizeof(statistics_interval));
	offset += sizeof(statistics_interval);

	// Store how many periods are recorded
	buffer[offset++] = STATISTICS_ELEMENTS;

	// And now copy over the struct
	memcpy(buffer + offset, &statistics_array, sizeof(struct statistics_element_t) * STATISTICS_ELEMENTS);
	offset += sizeof(struct statistics_element_t) * STATISTICS_ELEMENTS;

	return offset;
}

/**
 * \brief Resets the statistics, to be called by application after bundle has been retrieved
 */
void statistics_reset(void)
{
	PRINTF("STATISTICS: reset()\n");

	// Nullify the whole array
	memset(statistics_array, 0, sizeof(struct statistics_element_t) * STATISTICS_ELEMENTS);

	// Record the current timestamp
	statistics_timestamp = clock_seconds();
}

/**
 * \brief new bundle has been received
 */
void statistics_bundle_incoming(uint8_t count)
{
	if( statistics_interval < 1 ) {
		return;
	}

	PRINTF("STATISTICS: bundle_incoming(%u)\n", count);

	statistics_array[statistics_get_pointer()].bundles_incoming += count;
}

/**
 * \brief Bundle has been sent out
 */
void statistics_bundle_outgoing(uint8_t count)
{
	if( statistics_interval < 1 ) {
		return;
	}

	PRINTF("STATISTICS: bundle_outgoing(%u)\n", count);

	statistics_array[statistics_get_pointer()].bundles_outgoing += count;
}

/**
 * \brief Bundle was generated locally
 */
void statistics_bundle_generated(uint8_t count)
{
	if( statistics_interval < 1 ) {
		return;
	}

	PRINTF("STATISTICS: bundle_generated(%u)\n", count);

	statistics_array[statistics_get_pointer()].bundles_generated += count;
}

/**
 * \brief Bundle was delivered locally
 */
void statistics_bundle_delivered(uint8_t count)
{
	if( statistics_interval < 1 ) {
		return;
	}

	PRINTF("STATISTICS: statistics_bundle_delivered(%u)\n", count);

	statistics_array[statistics_get_pointer()].bundles_delivered += count;
}

/**
 * \brief Provide the number of bundles currently in storage
 */
void statistics_storage_bundles(uint8_t bundles)
{
	if( statistics_interval < 1 ) {
		return;
	}

	PRINTF("STATISTICS: storage_bundles(%u)\n", bundles);

	statistics_array[statistics_get_pointer()].storage_bundles = bundles;
}

/**
 * \brief Provide the amount of memory that is free
 */
void statistics_storage_memory(uint16_t free)
{
	if( statistics_interval < 1 ) {
		return;
	}

	PRINTF("STATISTICS: storage_memory(%u)\n", free);

	statistics_array[statistics_get_pointer()].storage_memory = free;
}

/**
 * \brief A new neighbour has been found
 */
void statistics_contacts_up(rimeaddr_t * peer)
{
	if( statistics_interval < 1 ) {
		return;
	}

	PRINTF("STATISTICS: contacts_up(%u.%u)\n", peer->u8[0], peer->u8[1]);
}

/**
 * \brief Neighbour disappeared
 */
void statistics_contacts_down(rimeaddr_t * peer, uint16_t duration)
{
	if( statistics_interval < 1 ) {
		return;
	}

	PRINTF("STATISTICS: contacts_down(%u.%u, %u)\n", peer->u8[0], peer->u8[1], duration);

	statistics_array[statistics_get_pointer()].contacts_count ++;
	statistics_array[statistics_get_pointer()].contacts_duration += duration;
}
/** @} */
