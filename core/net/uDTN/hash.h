/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup hashing Hashing modules
 *
 * @{
 */

/**
 * \file This file defines the interface for Hashing modules
 * \author Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef HASH_H
#define HASH_H

/** storage module interface  */
struct hash_driver {
	char *name;

	/** called by agent a startup */
	void (* init)(void);

	/** hashes the 4 arguments into 1 32 bit number */
	uint32_t (* hash_convenience)(uint32_t one, uint32_t two, uint32_t three, uint32_t four);

	/** hashes the 4 arguments into 1 32 bit number */
	uint32_t (* hash_convenience_ptr)(uint32_t * one, uint32_t * two, uint32_t * three, uint32_t * four);

	/** hashes the buffer into 1 32 bit number */
	uint32_t (* hash_buffer)(uint8_t * buffer, uint16_t length);
};

extern const struct hash_driver HASH;

#endif
/** @} */
/** @} */
