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

#include "system_clock.h"
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
	udtn_timeval_t tv;

	if( bundlemem == NULL ) {
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		return 0;
	}

	// Bundle has a timestamp and we have time sync
	if( bundle->tstamp != 0 && udtn_getclockstate() >= UDTN_CLOCK_STATE_GOOD ) {
		// Get current time
		udtn_gettimeofday(&tv);

		// Convert into DTN time
		tv.tv_sec -= UDTN_CLOCK_DTN_EPOCH_OFFSET;

		// If our clock is ahead of the bundle timestamp the age seems to be 0
		if( tv.tv_sec < bundle->tstamp ) {
			return 0;
		}

		// Calculate age based on timestamp and current time
		return (tv.tv_sec - bundle->tstamp) * 1000 + tv.tv_usec / 1000;
	}

	// We have to rely on the age block information
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

	if( bundlemem == NULL ) {
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		return 0;
	}

	/* Check age based on age block */
	age = bundle_ageing_get_age(bundlemem);

	if( (age / 1000) > bundle->lifetime ) {
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

#if UDTN_SUPPORT_LONG_AEB
	/* Decode the age block value */
	if( sdnv_len(buffer) > 4 ) {
		// 64 bit operations are expensive - avoid them where possible
		uint64_t age = 0;
		offset = sdnv_decode_long(buffer, length, &age);

		// Convert Age to milliseconds
		bundle->aeb_value_ms = (uint32_t) (age / 1000);
	} else {
		uint32_t age = 0;
		offset = sdnv_decode(buffer, length, &age);

		// Convert Age to milliseconds
		bundle->aeb_value_ms = age / 1000;
	}
#else
	uint32_t age = 0;
	offset = sdnv_decode(buffer, length, &age);

	// Convert Age to milliseconds
	bundle->aeb_value_ms = age / 1000;
#endif

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
	uint32_t flags = 0;
	int ret;

	if( bundlemem == NULL ) {
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	if( bundle == NULL ) {
		return 0;
	}

#if UDTN_SUPPORT_LONG_AEB
	/* Update the age value
	 * 4294967 = 0xFFFFFFFF / 1000
	 */
	if( bundle_ageing_get_age(bundlemem) > 4294967 ) {
		// Keep use of 64 bit data types as low as possible for performance reasons
		uint64_t age = 0;
		age = ((uint64_t) bundle_ageing_get_age(bundlemem)) * ((uint64_t) 1000);
		length = sdnv_encode_long(age, tmpbuffer, 10);
	} else {
		uint32_t age = 0;
		age = bundle_ageing_get_age(bundlemem) * 1000;
		length = sdnv_encode(age, tmpbuffer, 10);
	}
#else
	uint32_t age = 0;
	age = bundle_ageing_get_age(bundlemem) * 1000;
	length = sdnv_encode(age, tmpbuffer, 10);
#endif

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
