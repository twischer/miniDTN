/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup hash Hashing modules
 *
 * @{
 */

/**
 * \file
 * \brief This file defines the interface for Hashing modules
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef HASH_H
#define HASH_H

#include "contiki.h"

/**
 * Which hash driver are we going to use?
 */
#ifdef CONF_HASH
#define HASH CONF_HASH
#else
#define HASH hash_xxfast
#endif

/** storage module interface  */
struct hash_driver {
	char *name;

	/** called by agent a startup */
	void (* init)(void);

	/** hashes the 4 arguments into 1 32 bit number */
	uint32_t (* hash_convenience)(uint32_t one, uint32_t two, uint32_t three, uint32_t four, uint32_t five, uint32_t six);

	/** hashes the 4 arguments into 1 32 bit number */
	uint32_t (* hash_convenience_ptr)(uint32_t * one, uint32_t * two, uint32_t * three, uint32_t * four, uint32_t * five, uint32_t * six);

	/** hashes the buffer into 1 32 bit number */
	uint32_t (* hash_buffer)(uint8_t * buffer, uint16_t length);
};

extern const struct hash_driver HASH;

#endif
/** @} */
/** @} */
