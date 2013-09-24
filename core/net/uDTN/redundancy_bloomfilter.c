/**
 * \addtogroup redundancy
 * @{
 */

/**
 * \defgroup redundancy_bloomfilter Bloomfilter-based redundancy module
 *
 * @{
 */

/**
 * \file
 * \brief Implementation of a bloomfilter-based redundancy module
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // memset

#include "contiki.h"
#include "logging.h"

#include "hash.h"
#include "redundancy.h"
#include "agent.h"

/**
 * How much RAM can we invest into detecting redundant bundles?
 */
#ifdef CONF_REDUNDANCE_SIZE
#define REDUNDANCE_SIZE CONF_REDUNDANCE_SIZE
#else
#define REDUNDANCE_SIZE 128
#endif

/**
 * How many bloomfilter shall we use in parallel?
 */
#ifdef CONF_REDUNDANCE_NUMBER
#define REDUNDANCE_NUMBER CONF_REDUNDANCE_NUMBER
#else
#define REDUNDANCE_NUMBER 2
#endif

/**
 * How many bundles should go into each of the filters?
 */
#ifdef CONF_REDUNDANCE_LIMIT
#define REDUNDANCE_LIMIT CONF_REDUNDANCE_LIMIT
#else
#define REDUNDANCE_LIMIT 100
#endif

#define REDUNDANCE_PER_FILTER ((REDUNDANCE_SIZE / REDUNDANCE_NUMBER) * 8)

/**
 * Holds the bloomfilter(s)
 */
char redundance_filters[REDUNDANCE_SIZE];

/**
 * Counts the number of bundles in a bloom filter
 */
uint8_t redundance_counter = 0;

/**
 * Points to the filter that is currently in use
 */
uint8_t redundance_pointer = 0;

/**
 * \brief Get the length of each bloomfilter
 * \return Length of each bloomfilter
 */
uint32_t bloomfilter_get_length() {
	return REDUNDANCE_SIZE / REDUNDANCE_NUMBER;
}

/**
 * \brief Returns the pointer to the start of a bloom filter
 * \param filter Number of the bloomfilter
 * \return Pointer to the filter
 */
char * bloomfilter_get_start(uint8_t filter) {
	if( filter >= REDUNDANCE_NUMBER) {
		return NULL;
	}

	return &redundance_filters[filter * bloomfilter_get_length()];
}

/**
 * \brief Sets a bit in a given bloom filter
 * \param bit Offset of the bit to set
 * \param filter Number of the bloomfilter
 */
void bloomfilter_set(uint32_t bit, uint8_t filter) {
	uint8_t offset = 0;
	char * bloomfilter = NULL;

	bit = bit % REDUNDANCE_PER_FILTER;

	// Calculate the byte where the bit is in
	offset = bit / 8;

	// Calculate the bit within the byte
	bit = bit % 8;

	// Get the pointer to the beginning of the filter
	bloomfilter = bloomfilter_get_start(filter);

	if( bloomfilter == NULL ) {
		return;
	}

	// Set the proper bit
	*(bloomfilter + offset) |= 1 << bit;
}

/**
 * \brief Checks if a certain bit is set in a bloom filter
 * \param bit Offset of the bit to check
 * \param filter Number of the filter to look into
 * \return 1 if set, 0 otherwise
 */
uint8_t bloomfilter_get(uint32_t bit, uint8_t filter) {
	uint8_t offset = 0;
	char * bloomfilter = NULL;

	bit = bit % REDUNDANCE_PER_FILTER;

	// Calculate the byte where the bit is in
	offset = bit / 8;

	// Calculate the bit within the byte
	bit = bit % 8;

	// Get the pointer to the beginning of the filter
	bloomfilter = bloomfilter_get_start(filter);

	if( bloomfilter == NULL ) {
		return 0;
	}

	if( *(bloomfilter + offset) & (1 << bit) ) {
		return 1;
	}

	return 0;
}

/**
 * \brief Clears the content of a bloom filter
 * \param filter Number of the bloom filter
 */
void bloomfilter_clear(uint8_t filter) {
	memset((void *) bloomfilter_get_start(filter), 0, bloomfilter_get_length());
}

/**
 * \brief checks if bundle was delivered before
 * \param bundlemem pointer to bundle
 * \return 1 if bundle was delivered before, 0 otherwise
 */
uint8_t redundancy_bloomfilter_check(uint32_t bundle_number)
{
	uint16_t hash1 = 0;
	uint16_t hash2 = 0;
	int i;

	// Split our bundle number into 2 hashes
	hash1 = (bundle_number & 0x0000FFFF) >> 0;
	hash2 = (bundle_number & 0xFFFF0000) >> 16;

	// Now go and check the filters
	for(i=0; i<REDUNDANCE_NUMBER; i++) {
		if( bloomfilter_get(hash1, i) && bloomfilter_get(hash2, i) ) {
			return 1;
		}
	}

	// If the bundle was found in no filter, it has not been seen before
	return 0;
}

/**
 * \brief saves the bundle in a list of delivered bundles
 * \param bundlemem pointer to bundle
 * \return 1 on successor 0 on error
 */
uint8_t redundancy_bloomfilter_set(uint32_t bundle_number)
{
	uint16_t hash1 = 0;
	uint16_t hash2 = 0;

	// Split our bundle number into 2 hashes
	hash1 = (bundle_number & 0x0000FFFF) >> 0;
	hash2 = (bundle_number & 0xFFFF0000) >> 16;

	// Set them in the current filter
	bloomfilter_set(hash1, redundance_pointer);
	bloomfilter_set(hash2, redundance_pointer);

	// Increment the number of set bundles
	redundance_counter ++;

	// Eventually switch to the next filter, if the previous one is filled up
	if( redundance_counter > REDUNDANCE_LIMIT ) {
		redundance_pointer = (redundance_pointer + 1) % REDUNDANCE_NUMBER;
		bloomfilter_clear(redundance_pointer);
		redundance_counter = 0;
	}

	return 1;
}
		
/**
 * \brief Initialize the bloomfilters and various other state
 */
void redundancy_bloomfilter_init(void)
{
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "REDUNDANCE: starting");

	// Clear the filters
	memset(redundance_filters, 0, REDUNDANCE_SIZE);
	redundance_counter = 0;
	redundance_pointer = 0;
}

const struct redundance_check redundancy_bloomfilter ={
	"REDUNDANCE_BLOOMFILTER",
	redundancy_bloomfilter_init,
	redundancy_bloomfilter_check,
	redundancy_bloomfilter_set,
};
/** @} */
/** @} */
