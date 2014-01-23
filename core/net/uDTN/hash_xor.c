/**
 * \addtogroup hash
 * @{
 */

/**
 * \defgroup hash_xor 'Hashing' implementation using the XOR function
 *
 * @{
 */

/**
 * \file
 * \brief Implementation of the hashing interface based on the XOR function
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdio.h>
#include <string.h> // memcpy

#include "hash.h"

void hash_xor_init() {
}

uint32_t hash_xor_buffer(uint8_t * buffer, uint16_t length)
{
	int i;
	int limit = (length / 4) * 4;
	uint32_t hash = 0;

	for( i=0; i < limit; i = i + 4) {
		uint32_t * value = (uint32_t* ) &buffer[i];
		hash ^= *value;
	}

	if( limit < length ) {
		uint32_t remainder = 0;
		memcpy(&remainder, &buffer[limit], length - limit);
		hash ^= remainder;
	}

	return hash;
}

uint32_t hash_xor_convenience(uint32_t one, uint32_t two, uint32_t three, uint32_t four, uint32_t five, uint32_t six)
{
	uint8_t buffer[24];

	memcpy(buffer + 0, &one, sizeof(uint32_t));
	memcpy(buffer + 4, &two, sizeof(uint32_t));
	memcpy(buffer + 8, &three, sizeof(uint32_t));
	memcpy(buffer + 12, &four, sizeof(uint32_t));
	memcpy(buffer + 16, &five, sizeof(uint32_t));
	memcpy(buffer + 20, &six, sizeof(uint32_t));

	return hash_xor_buffer(buffer, 24);
}

uint32_t hash_xor_convenience_ptr(uint32_t * one, uint32_t * two, uint32_t * three, uint32_t * four, uint32_t * five, uint32_t * six)
{
	uint8_t buffer[24];

	memcpy(buffer + 0, one, sizeof(uint32_t));
	memcpy(buffer + 4, two, sizeof(uint32_t));
	memcpy(buffer + 8, three, sizeof(uint32_t));
	memcpy(buffer + 12, four, sizeof(uint32_t));
	memcpy(buffer + 16, five, sizeof(uint32_t));
	memcpy(buffer + 20, six, sizeof(uint32_t));

	return hash_xor_buffer(buffer, 24);
}

const struct hash_driver hash_xor = {
	"XOR",
	hash_xor_init,
	hash_xor_convenience,
	hash_xor_convenience_ptr,
	hash_xor_buffer,
};

/** @} */
/** @} */
