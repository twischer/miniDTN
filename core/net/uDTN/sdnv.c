 /**
 * \addtogroup sdnv SDNV Functions
 *
 * @{
 */

/**
 * \file 
 * \brief implementation of SDNV functions
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lib/logging.h"

#include "agent.h"

#include "sdnv.h"

/** maximum sdnv length */
#define MAX_LENGTH 5
#define MAX_LENGTH_LONG 10

int sdnv_encode_long(uint64_t val, uint8_t* bp, size_t len)
{
	size_t val_len = 0;
	uint32_t tmp = val;

	do {
		tmp = tmp >> 7;
		val_len++;
	} while (tmp != 0);


	if (len < val_len) {
		return -1;
	}

	bp += val_len;
	uint8_t high_bit = 0; // for the last octet
	do {
		--bp;
		*bp = (uint8_t)(high_bit | (val & 0x7f));
		high_bit = (1 << 7); // for all but the last octet
		val = val >> 7;
	} while (val != 0);

	return val_len;
}

int sdnv_encode(uint32_t val, uint8_t* bp, size_t len)
{
	size_t val_len = 0;
	uint32_t tmp = val;

	do {
		tmp = tmp >> 7;
		val_len++;
	} while (tmp != 0);


	if (len < val_len) {
		return -1;
	}

	bp += val_len;
	uint8_t high_bit = 0; // for the last octet
	do {
		--bp;
		*bp = (uint8_t)(high_bit | (val & 0x7f));
		high_bit = (1 << 7); // for all but the last octet
		val = val >> 7;
	} while (val != 0);

	return val_len;
}

size_t sdnv_encoding_len_long(uint64_t val)
{
	size_t val_len = 0;

	do {
		val = val >> 7;
		val_len++;
	} while (val != 0);

	return val_len;
}

size_t sdnv_encoding_len(uint32_t val)
{
	size_t val_len = 0;
	//	printf("val %lu ptr %p\n ",val,&val);
	do {
		val = val >> 7;
		val_len++;
	} while (val != 0);

	return val_len;
}

int sdnv_decode_long(const uint8_t* bp, size_t len, uint64_t* val)
{
	const uint8_t* start = bp;
	size_t val_len = 0;
	*val = 0;

	LOG(LOGD_DTN, LOG_SDNV, LOGL_DBG, "sdnv_decode");

	if (!val) {
		LOG(LOGD_DTN, LOG_SDNV, LOGL_ERR, "SDNV: NULL pointer");
		return -1;
	}

	do {
		LOG(LOGD_DTN, LOG_SDNV, LOGL_DBG, "SDNV: len: %u", len);
		if (len == 0){
			LOG(LOGD_DTN, LOG_SDNV, LOGL_ERR, "SDNV: buffer too short");
			return sdnv_len(start); // buffer too short
		}
		*val = (*val << 7) | (*bp & 0x7f);
		++val_len;

		if ((*bp & (1 << 7)) == 0){
			break; // all done;
		}

		++bp;
		--len;
	} while (1);

	if ((val_len > MAX_LENGTH_LONG) || ((val_len == MAX_LENGTH_LONG) && (*start > 0x81))){
		LOG(LOGD_DTN, LOG_SDNV, LOGL_ERR, "SDNV: val_len >= %u", MAX_LENGTH_LONG);
		*val = 0;
		return sdnv_len(start);
	}

	LOG(LOGD_DTN, LOG_SDNV, LOGL_DBG, "SDNV: val: %lu", *val);
	return val_len;
}

int sdnv_decode(const uint8_t* bp, size_t len, uint32_t* val)
{
	configASSERT(IS_RAM(bp) && IS_RAM(val));

	LOG(LOGD_DTN, LOG_SDNV, LOGL_DBG, "sdnv_decode(in %p, len %lu, out %p)", bp, len, val);
	const uint8_t* start = bp;
	if (!val) {
		LOG(LOGD_DTN, LOG_SDNV, LOGL_ERR, "SDNV: NULL pointer");
		return -1;
	}

	size_t val_len = 0;
	*val = 0;
	do {
		LOG(LOGD_DTN, LOG_SDNV, LOGL_DBG, "SDNV: len: %u", len);
		if (len == 0){
			LOG(LOGD_DTN, LOG_SDNV, LOGL_ERR, "SDNV: buffer too short");
			val = 0;
			return sdnv_len(start); // buffer too short
		}
		*val = (*val << 7) | (*bp & 0x7f);
		++val_len;

		if ((*bp & (1 << 7)) == 0){
			break; // all done;
		}

		++bp;
		--len;
	} while (1);

	/* failes, if more than 32 value bits available
	 * Only the first 4 bits of the first byte should be set,
	 * if a 5 byte SDNV was processed
	 */
	if ((val_len > MAX_LENGTH) || ((val_len == MAX_LENGTH) && (*start > 0x8F))){
		LOG(LOGD_DTN, LOG_SDNV, LOGL_ERR, "SDNV: val_len >= %u (%02x %02x %02x %02x ...)",
			MAX_LENGTH, start[0], start[1], start[2], start[3]);
		*val = 0;
		return sdnv_len(start);
	}

	LOG(LOGD_DTN, LOG_SDNV, LOGL_DBG, "SDNV: val: %lu", *val);
	return val_len;
}

size_t sdnv_len(const uint8_t* bp)
{
	size_t val_len = 1;
	for ( ; *bp++ & 0x80; ++val_len )
		;
	return val_len;
}
/** @} */



