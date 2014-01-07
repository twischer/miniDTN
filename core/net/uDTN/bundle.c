/**
 * \addtogroup bundle
 *
 * @{
 */

/**
 * \file
 * \brief this file implements the bundle memory representation
 *
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmem.h"
#include "sys/process.h"
#include "clock.h"
#include "logging.h"

#include "sdnv.h"
#include "bundleslot.h"
#include "agent.h"
#include "bundle_ageing.h"

#include "bundle.h"

/**
 * "Internal" functions
 */
static uint8_t bundle_decode_block(struct mmem *bundlemem, uint8_t *buffer, int max_len);
static int bundle_encode_block(struct bundle_block_t *block, uint8_t *buffer, uint8_t max_len);


struct mmem * bundle_create_bundle()
{
	int ret;
	struct bundle_slot_t *bs;
	struct bundle_t *bundle;

	bs = bundleslot_get_free();

	if( bs == NULL ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Could not allocate slot for a bundle");
		return NULL;
	}

	ret = mmem_alloc(&bs->bundle, sizeof(struct bundle_t));
	if (!ret) {
		bundleslot_free(bs);
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Could not allocate memory for a bundle");
		return NULL;
	}

	bundle = (struct bundle_t *) MMEM_PTR(&bs->bundle);
	memset(bundle, 0, sizeof(struct bundle_t));
	bundle->rec_time = (uint32_t) clock_time();
	bundle->num_blocks = 0;
	bundle->source_process = PROCESS_CURRENT();

	return &bs->bundle;
}

int bundle_add_block(struct mmem *bundlemem, uint8_t type, uint8_t flags, uint8_t *data, uint8_t d_len)
{
	struct bundle_t *bundle;
	struct bundle_block_t *block;
	uint8_t i;
	int n;

	n = mmem_realloc(bundlemem, bundlemem->size + d_len + sizeof(struct bundle_block_t));
	if( !n ) {
		return -1;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	/* FIXME: Make sure we don't traverse outside of our allocated memory */

	/* Go through the blocks until we're behind the last one */
	block = (struct bundle_block_t *) bundle->block_data;
	for (i=0;i<bundle->num_blocks;i++) {
		/* None of these is the last block anymore */
		block->flags &= ~BUNDLE_BLOCK_FLAG_LAST;
		block = (struct bundle_block_t *) &block->payload[block->block_size];
	}

	block->type = type;
	block->flags = BUNDLE_BLOCK_FLAG_LAST | flags;
	block->block_size = d_len;

	bundle->num_blocks++;

	memcpy(block->payload, data, d_len);

	return d_len;
}

struct bundle_block_t *bundle_get_block(struct mmem *bundlemem, uint8_t i)
{
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	struct bundle_block_t *block = (struct bundle_block_t *) bundle->block_data;

	if (i >= bundle->num_blocks)
		return NULL;

	for (;i!=0;i--) {
		block = (struct bundle_block_t *) &block->payload[block->block_size];
	}

	return block;
}

struct bundle_block_t *bundle_get_block_by_type(struct mmem *bundlemem, uint8_t type)
{
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	struct bundle_block_t *block = (struct bundle_block_t *) bundle->block_data;
	int i = 0;

	for(i=0; i<bundle->num_blocks; i++) {
		if( block->type == type ) {
			return block;
		}

		block = (struct bundle_block_t *) &block->payload[block->block_size];
	}

	return NULL;
}

struct bundle_block_t * bundle_get_payload_block(struct mmem * bundlemem) {
	return bundle_get_block_by_type(bundlemem, BUNDLE_BLOCK_TYPE_PAYLOAD);
}

uint8_t bundle_set_attr(struct mmem *bundlemem, uint8_t attr, uint32_t *val)
{
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "set attr %lx",*val);
	switch (attr) {
		case FLAGS:
			bundle->flags = *val;
			bundle->custody = 0x08 &(uint8_t) *val;
			break;
		case DEST_NODE:
			bundle->dst_node = *val;
			break;
		case DEST_SERV:
			bundle->dst_srv = *val;
			break;
		case SRC_NODE:
			bundle->src_node = *val;
			break;
		case SRC_SERV:
			bundle->src_srv = *val;
			break;
		case REP_NODE:
			bundle->rep_node = *val;
			break;
		case REP_SERV:
			bundle->rep_srv = *val;
			break;
		case CUST_NODE:
			bundle->cust_node = *val;
			break;
		case CUST_SERV:
			bundle->cust_srv = *val;
			break;
		case TIME_STAMP:
			bundle->tstamp = *val;
			break;
		case TIME_STAMP_SEQ_NR:
			bundle->tstamp_seq = *val;
			break;
		case LIFE_TIME:
			bundle->lifetime = *val;
			break;
		case DIRECTORY_LEN:
			if (*val != 0)
				LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Dictionary length needs to be 0 for CBHE");
			break;
		case FRAG_OFFSET:
			bundle->frag_offs = *val;
			break;
		case LENGTH:
			/* FIXME */
		default:
			LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Unknown attribute");
			return 0;
	}
	return 1;
}

uint8_t bundle_get_attr(struct mmem *bundlemem, uint8_t attr, uint32_t *val)
{
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "get attr: %d in %lx", attr, *val);
	switch (attr) {
		case FLAGS:
			*val = bundle->flags;
			break;
		case DEST_NODE:
			*val = bundle->dst_node;
			break;
		case DEST_SERV:
			*val = bundle->dst_srv;
			break;
		case SRC_NODE:
			*val = bundle->src_node;
			break;
		case SRC_SERV:
			*val = bundle->src_srv;
			break;
		case REP_NODE:
			*val = bundle->rep_node;
			break;
		case REP_SERV:
			*val = bundle->rep_srv;
			break;
		case CUST_NODE:
			*val = bundle->cust_node;
			break;
		case CUST_SERV:
			*val = bundle->cust_srv;
			break;
		case TIME_STAMP:
			*val = bundle->tstamp;
			break;
		case TIME_STAMP_SEQ_NR:
			*val = bundle->tstamp_seq;
			break;
		case LIFE_TIME:
			*val = bundle->lifetime;
			break;
		case DIRECTORY_LEN:
			*val = 0;
			break;
		case FRAG_OFFSET:
			*val = bundle->frag_offs;
			break;
		case LENGTH:
			/* FIXME */
		default:
			LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Unknown attribute");
			return 0;
	}
	return 1;
}

struct mmem *bundle_recover_bundle(uint8_t *buffer, int size)
{
	uint32_t primary_size, value;
	uint8_t offs = 0;
	struct mmem *bundlemem;
	struct bundle_t *bundle;
	uint8_t ret = 0;

	bundlemem = bundle_create_bundle();
	if (!bundlemem)
		return NULL;

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "rec bptr: %p  blptr:%p",bundle,buffer);

	/* Version 0x06 is the one described and supported in RFC5050 */
	if (buffer[0] != 0x06) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Version 0x%02x not supported", buffer[0]);
		goto err;
	}
	offs++;

	/* Flags */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->flags);

	/* Block Length - Number of bytes in this block following this
	 * field */
	offs += sdnv_decode(&buffer[offs], size-offs, &primary_size);
	primary_size += offs;

	/* Destination node + SSP */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->dst_node);
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->dst_srv);

	/* Source node + SSP */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->src_node);
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->src_srv);

	/* Report-to node + SSP */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->rep_node);
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->rep_srv);

	/* Custodian node + SSP */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->cust_node);
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->cust_srv);

	/* Creation Timestamp */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->tstamp);

	/* Creation Timestamp Sequence Number */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->tstamp_seq);

	/* Lifetime */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->lifetime);

	/* Directory Length */
	offs += sdnv_decode(&buffer[offs], size-offs, &value);
	if (value != 0) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Bundle does not use CBHE.");
		goto err;
	}

	if (bundle->flags & BUNDLE_FLAG_FRAGMENT) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_INF, "Bundle is a fragment");

		/* Fragment Offset */
		offs += sdnv_decode(&buffer[offs], size-offs, &bundle->frag_offs);

		/* Total App Data Unit Length */
		offs += sdnv_decode(&buffer[offs], size-offs, &bundle->app_len);
	}

	if (offs != primary_size) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Problem decoding the primary bundle block.");
		goto err;
	}

	/* FIXME: Loop around and decode all blocks - does this work? */
	while (size-offs > 1) {
		ret = bundle_decode_block(bundlemem, &buffer[offs], size-offs);

		/* If block decode failed, we are out of memory and have to abort */
		if( ret < 1 ) {
			goto err;
		}

		offs += ret;
	}

	return bundlemem;

err:
	bundle_delete_bundle(bundlemem);
	return NULL;

}

static uint8_t bundle_decode_block(struct mmem *bundlemem, uint8_t *buffer, int max_len)
{
	uint8_t type, block_offs, offs = 0;
	uint32_t flags, size;
	struct bundle_t *bundle;
	struct bundle_block_t *block;
	int n;

	type = buffer[offs];
	offs++;

	/* Flags */
	offs += sdnv_decode(&buffer[offs], max_len-offs, &flags);

	/* Payload Size */
	offs += sdnv_decode(&buffer[offs], max_len-offs, &size);
	if (size > max_len-offs) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Bundle payload length too big.");
		return 0;
	}

	block_offs = bundlemem->size;

	if( type == BUNDLE_BLOCK_TYPE_AEB ) {
		return offs + bundle_ageing_parse_age_extension_block(bundlemem, type, flags, &buffer[offs], size);
	}

	n = mmem_realloc(bundlemem, bundlemem->size + sizeof(struct bundle_block_t) + size);
	if( !n ) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_ERR, "Bundle payload length too big for MMEM.");
		return 0;
	}

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	bundle->num_blocks++;

	/* Add the block to the end of the bundle */
	block = (struct bundle_block_t *)((uint8_t *)bundle + block_offs);
	block->type = type;
	block->flags = flags;
	block->block_size = size;

	/* Copy the actual payload over */
	memcpy(block->payload, &buffer[offs], block->block_size);

	return offs + block->block_size;
}

int bundle_encode_bundle(struct mmem *bundlemem, uint8_t *buffer, int max_len)
{
	uint32_t value;
	uint8_t offs = 0, blklen_offs, i;
	int ret, blklen_size;
	struct bundle_t *bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	struct bundle_block_t *block;

	/* Hardcode the version to 0x06 */
	buffer[0] = 0x06;
	offs++;

	/* Flags */
	ret = sdnv_encode(bundle->flags, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Block length will be calculated later
	 * reserve one byte for now */
	blklen_offs = offs;
	offs++;

	/* Destination node + SSP */
	ret = sdnv_encode(bundle->dst_node, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	ret = sdnv_encode(bundle->dst_srv, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Source node + SSP */
	ret = sdnv_encode(bundle->src_node, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	ret = sdnv_encode(bundle->src_srv, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Report-to node + SSP */
	ret = sdnv_encode(bundle->rep_node, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	ret = sdnv_encode(bundle->rep_srv, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Custodian node + SSP */
	ret = sdnv_encode(bundle->cust_node, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	ret = sdnv_encode(bundle->cust_srv, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Creation Timestamp */
	ret = sdnv_encode(bundle->tstamp, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Creation Timestamp Sequence Number */
	ret = sdnv_encode(bundle->tstamp_seq, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Lifetime */
	ret = sdnv_encode(bundle->lifetime, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Directory Length */
	ret = sdnv_encode(0l, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	if (bundle->flags & BUNDLE_FLAG_FRAGMENT) {
		LOG(LOGD_DTN, LOG_BUNDLE, LOGL_INF, "Bundle is a fragment");

		/* Fragment Offset */
		ret = sdnv_encode(bundle->frag_offs, &buffer[offs], max_len - offs);
		if (ret < 0)
			return -1;
		offs += ret;

		/* Total App Data Unit Length */
		ret = sdnv_encode(bundle->app_len, &buffer[offs], max_len - offs);
		if (ret < 0)
			return -1;
		offs += ret;
	}

	/* Calculate block length value */
	value = offs - blklen_offs - 1;
	blklen_size = sdnv_encoding_len(value);
	/* Move the data around */
	if (blklen_size > 1) {
		memmove(&buffer[blklen_offs+blklen_size], &buffer[blklen_offs+1], value);
	}
	ret = sdnv_encode(value, &buffer[blklen_offs], blklen_size);

	offs += ret-1;

	/* Encode Bundle Age Block - always as first block */
	offs += bundle_ageing_encode_age_extension_block(bundlemem, &buffer[offs], max_len - offs);

	block = (struct bundle_block_t *) bundle->block_data;
	for (i=0;i<bundle->num_blocks;i++) {
		offs += bundle_encode_block(block, &buffer[offs], max_len - offs);

		/* Reference the next block */
		block = (struct bundle_block_t *) &block->payload[block->block_size];
	}

	return offs;
}

static int bundle_encode_block(struct bundle_block_t *block, uint8_t *buffer, uint8_t max_len)
{
	uint8_t offs = 0;
	int ret;
	uint32_t value;

	/* Encode the next block */
	buffer[offs] = block->type;
	offs++;

	/* Flags */
	ret = sdnv_encode(block->flags, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Blocksize */
	value = block->block_size;
	ret = sdnv_encode(value, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Payload */
	memcpy(&buffer[offs], block->payload, block->block_size);
	offs += block->block_size;

	return offs;
}

int bundle_increment(struct mmem *bundlemem)
{
	struct bundle_slot_t *bs;
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "bundle_increment(%p)", bundlemem);

	bs = container_of(bundlemem, struct bundle_slot_t, bundle);
	return bundleslot_increment(bs);
}

int bundle_decrement(struct mmem *bundlemem)
{
	struct bundle_slot_t *bs;
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "bundle_decrement(%p)", bundlemem);

	bs = container_of(bundlemem, struct bundle_slot_t, bundle);
	return bundleslot_decrement(bs);
}

uint16_t bundle_delete_bundle(struct mmem *bundlemem)
{
	struct bundle_slot_t *bs;
	LOG(LOGD_DTN, LOG_BUNDLE, LOGL_DBG, "delete %p", bundlemem);

	bs = container_of(bundlemem, struct bundle_slot_t, bundle);
	bundleslot_free(bs);
	return 1;
}

rimeaddr_t convert_eid_to_rime(uint32_t eid) {
	/* FIXME: Is this endian-specific? If so, we should fix it properly! */
	rimeaddr_t dest;
	dest.u8[0] = (eid & 0x000000FF) >> 0;
	dest.u8[1] = (eid & 0x0000FF00) >> 8;
	return dest;
}

uint32_t convert_rime_to_eid(rimeaddr_t * dest) {
	/* FIXME: Is this endian-specific? If so, we should fix it properly! */
	uint32_t eid = 0;
	eid = (dest->u8[0] & 0xFF) + ((dest->u8[1] & 0xFF) << 8);
	return eid;
}
