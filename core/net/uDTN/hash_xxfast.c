/**
 * \addtogroup hash
 * @{
 */

/**
 * \defgroup hash_xxfast Hashing implementation using the xxFAST algorithm
 *
 * @{
 */

/**
 * \file
 * \brief Implementation of the hashing interface based on the xxFAST algorithm
 * \author Yann Collet
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

/*
   xxHash - Fast Hash algorithm
   Copyright (C) 2012, Yann Collet.
   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	You can contact the author at :
	- xxHash source repository : http://code.google.com/p/xxhash/
*/

#include <stdio.h>
#include <string.h> // memcpy

#include "hash.h"

#define _rotl(x,r) ((x << r) | (x >> (32 - r)))

#define PRIME1   2654435761U
#define PRIME2   2246822519U
#define PRIME3   3266489917U
#define PRIME4    668265263U
#define PRIME5   0x165667b1

void hash_xxfast_init() {
}

uint32_t hash_xxfast_buffer(uint8_t * buffer, uint16_t length)
{
	uint8_t * p = buffer;
	uint8_t * bEnd = p + length;
	uint8_t * limit = bEnd - 4;
	uint32_t idx = PRIME1;
	uint32_t crc = PRIME5;

	while (p<limit)
	{
		crc += ((*(uint32_t *)p) + idx++);
		crc += _rotl(crc, 17) * PRIME4;
		crc *= PRIME1;
		p+=4;
	}

	while (p<bEnd)
	{
		crc += ((*p) + idx++);
		crc *= PRIME1;
		p++;
	}

	crc += length;

	crc ^= crc >> 15;
	crc *= PRIME2;
	crc ^= crc >> 13;
	crc *= PRIME3;
	crc ^= crc >> 16;

	return crc;
}

uint32_t hash_xxfast_convenience(uint32_t one, uint32_t two, uint32_t three, uint32_t four, uint32_t five, uint32_t six)
{
	uint8_t buffer[24];

	memcpy(buffer + 0, &one, sizeof(uint32_t));
	memcpy(buffer + 4, &two, sizeof(uint32_t));
	memcpy(buffer + 8, &three, sizeof(uint32_t));
	memcpy(buffer + 12, &four, sizeof(uint32_t));
	memcpy(buffer + 16, &five, sizeof(uint32_t));
	memcpy(buffer + 20, &six, sizeof(uint32_t));

	return hash_xxfast_buffer(buffer, 24);
}

uint32_t hash_xxfast_convenience_ptr(uint32_t * one, uint32_t * two, uint32_t * three, uint32_t * four, uint32_t * five, uint32_t * six)
{
	uint8_t buffer[24];

	memcpy(buffer + 0, one, sizeof(uint32_t));
	memcpy(buffer + 4, two, sizeof(uint32_t));
	memcpy(buffer + 8, three, sizeof(uint32_t));
	memcpy(buffer + 12, four, sizeof(uint32_t));
	memcpy(buffer + 16, five, sizeof(uint32_t));
	memcpy(buffer + 20, six, sizeof(uint32_t));

	return hash_xxfast_buffer(buffer, 24);
}

const struct hash_driver hash_xxfast = {
	"xxFAST",
	hash_xxfast_init,
	hash_xxfast_convenience,
	hash_xxfast_convenience_ptr,
	hash_xxfast_buffer,
};

/** @} */
/** @} */
