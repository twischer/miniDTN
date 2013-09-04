/**
 * \addtogroup agent
 * @{
 */

/**
 * \file
 * \brief This file is responsible for bundle ageing mechanisms
 *
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <string.h>

#include "bundle.h"
#include "agent.h"
#include "sdnv.h"

/**
 * \brief Get the age of the bundle
 * \param bundlemem Bundle MMEM Pointer
 * \return Age in milliseconds
 */
uint32_t bundle_ageing_get_age(struct mmem * bundlemem) {
	struct bundle_t *bundle;

	if( bundlemem == NULL ) {
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		return 0;
	}

	return bundle->aeb_value_ms + (((uint32_t) clock_time() - bundle->rec_time) * 1000) / CLOCK_SECOND;
}

/**
 * \brief Figure out if the bundle is expired
 * \param bundlemem Bundle MMEM Pointer
 * \return 1 = expired, 0 = not expired
 */
uint8_t bundle_ageing_is_expired(struct mmem * bundlemem) {
	struct bundle_t *bundle;
	uint32_t age = 0;
	uint32_t lifetime = 0;

	if( bundlemem == NULL ) {
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		return 0;
	}

	age = bundle_ageing_get_age(bundlemem);
	lifetime = bundle->lifetime;

	if( (age / 1000) >= lifetime ) {
		return 1;
	}

	return 0;
}

/**
 * \brief Parses the age extension block
 * \param bundlemem Bundle MMEM Pointer
 * \param type Block type
 * \param flags Block Flags
 * \param buffer Block Payload Pointer
 * \param length Block Payload Length
 * \return Length of parsed block payload
 */
uint8_t bundle_ageing_parse_age_extension_block(struct mmem *bundlemem, uint8_t type, uint32_t flags, uint8_t * buffer, int length) {
	uint8_t offset = 0;
	uint64_t age = 0;
	struct bundle_t *bundle;

	/* Check for the proper block type */
	if( type != BUNDLE_BLOCK_TYPE_AEB ) {
		return 0;
	}

	if( bundlemem == NULL ) {
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		return 0;
	}

	/* Decode the age block value */
	offset = sdnv_decode_long(buffer, length, &age);

	if( type == BUNDLE_BLOCK_TYPE_AEB ) {
		/* age is in microseconds... what a stupid idea */
		age /= 1000;
	}

	bundle->aeb_value_ms = (uint32_t) age;

	return offset;
}

/**
 * \brief Encodes the age extension block
 * \param bundlemem Bundle MMEM Pointer
 * \param buffer Block Payload Pointer
 * \param max_len Block Payload Length
 * \return Length of encoded block payload
 */
uint8_t bundle_ageing_encode_age_extension_block(struct mmem *bundlemem, uint8_t *buffer, int max_len) {
	struct bundle_t *bundle;
	uint32_t length = 0;
	uint8_t offset = 0;
	uint8_t tmpbuffer[10];
	uint64_t age = 0;
	uint32_t flags = 0;
	int ret;

	if( bundlemem == NULL ) {
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		return 0;
	}

	/* Update the age value */
	age = ((uint64_t) bundle_ageing_get_age(bundlemem)) * ((uint64_t) 1000);

	/* Encode the next block */
	buffer[offset] = BUNDLE_BLOCK_TYPE_AEB;
	offset++;

	/* Flags */
	flags = BUNDLE_BLOCK_FLAG_REPL;
	ret = sdnv_encode(flags, &buffer[offset], max_len - offset);
	if (ret < 0) {
		return 0;
	}
	offset += ret;

	/* Encode the age in ms */
	length = sdnv_encode_long(age, tmpbuffer, 10);

	/* Blocksize */
	ret = sdnv_encode(length, &buffer[offset], max_len - offset);
	if (ret < 0) {
		return 0;
	}
	offset += ret;

	/* Payload */
	memcpy(&buffer[offset], tmpbuffer, length);
	offset += length;

	return offset;
}
/** @} */
