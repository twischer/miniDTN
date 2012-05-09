#include "bundle.h"
#include "sdnv.h"
#include "mmem.h"
#include <stdlib.h>
#include <stdio.h>
#if CONTIKI_TARGET_SKY
//	#include "net/dtn/realloc.h"
#endif

#include <string.h>
#include "clock.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint8_t create_bundle(struct bundle_t *bundle)
{
	memset(bundle, 0, sizeof(struct bundle_t));

	bundle->rec_time=(uint32_t) clock_seconds();

	return 1;
}

uint8_t add_block(struct bundle_t *bundle, uint8_t type, uint8_t flags, uint8_t *data, uint8_t d_len)
{
	bundle->block.type = type;
	bundle->block.flags = BUNDLE_BLOCK_FLAG_LAST;
	bundle->block.block_size = d_len;

	if (!mmem_alloc(&bundle->block.payload, d_len)) {
		return 0;
	}

	memcpy(MMEM_PTR(&bundle->block.payload), data, d_len);

	return d_len;
}

struct bundle_block_t *get_block(struct bundle_t *bundle)
{
	return &bundle->block;
}

/**
*brief converts an integer value in sdnv and copies this to the right place in bundel
*/
#if DEBUG
void hexdump(char * string, uint8_t * bla, int length) {
	printf("Hex: (%s) ", string);
	int i;
	for(i=0; i<length; i++) {
		printf("%02X ", *(bla+i));
	}
	printf("\n");
}
#else
#define hexdump(...)
#endif

uint8_t set_attr(struct bundle_t *bundle, uint8_t attr, uint32_t *val)
{
	PRINTF("set attr %lx\n",*val);
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
				PRINTF("Dictionary length needs to be 0 for CBHE\n");
			break;
		case FRAG_OFFSET:
			bundle->frag_offs = *val;
			break;
		case LENGTH:
			/* FIXME */
		default:
			PRINTF("Unknown attribute\n");
			return 0;
	}
	return 1;
}

uint8_t get_attr(struct bundle_t *bundle, uint8_t attr, uint32_t *val)
{
	PRINTF("get attr %lx\n",*val);
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
			*val = bundle->tstamp = *val;
			break;
		case TIME_STAMP_SEQ_NR:
			*val = bundle->tstamp_seq = *val;
			break;
		case LIFE_TIME:
			*val = bundle->lifetime = *val;
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
			PRINTF("Unknown attribute\n");
			return 0;
	}
	return 1;
}

uint8_t recover_bundle(struct bundle_t *bundle, uint8_t *buffer, int size)
{
	uint32_t primary_size, value;
	uint8_t offs = 0;

	PRINTF("rec bptr: %p  blptr:%p \n",bundle,buffer);
	create_bundle(bundle);

	/* Version 0x06 is the one described and supported in RFC5050 */
	if (buffer[0] != 0x06) {
		PRINTF("Version 0x%02x not supported\n", buffer[0]);
		return 0;
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
		PRINTF("Bundle does not use CBHE.\n");
		return 0;
	}

	if (bundle->flags & BUNDLE_FLAG_FRAGMENT) {
		PRINTF("Bundle is a fragment\n");

		/* Fragment Offset */
		offs += sdnv_decode(&buffer[offs], size-offs, &bundle->frag_offs);

		/* Total App Data Unit Length */
		offs += sdnv_decode(&buffer[offs], size-offs, &bundle->app_len);
	}

	if (offs != primary_size) {
		PRINTF("Problem decoding the primary bundle block.\n");
		return 0;
	}

	/* Decode the next block
	 * FIXME: Only support for one block after the primary for now */

	bundle->block.type = buffer[offs];
	offs++;

	/* Flags */
	offs += sdnv_decode(&buffer[offs], size-offs, &bundle->block.flags);

	/* Payload Size */
	offs += sdnv_decode(&buffer[offs], size-offs, &value);
	if (value > 127) {
		PRINTF("Bundle payload length too big.\n");
		return 0;
	}
	bundle->block.block_size = value;

	if (bundle->block.block_size != size-offs) {
		printf("Bundle payload length %i doesn't match buffer size (%i, %i).\n", bundle->block.block_size, offs, size);
		return 0;
	}

	/* Copy the actual payload over */
	mmem_alloc(&bundle->block.payload, bundle->block.block_size);
	memcpy(MMEM_PTR(&bundle->block.payload), &buffer[offs], bundle->block.block_size);

	return 1;
}

uint8_t encode_bundle(struct bundle_t *bundle, uint8_t *buffer, int max_len)
{
	uint32_t value;
	uint8_t offs = 0, blklen_offs;
	int ret, blklen_size;

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
		PRINTF("Bundle is a fragment\n");

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

	/* Encode the next block */
	buffer[offs] = bundle->block.type;
	offs++;

	/* Flags */
	ret = sdnv_encode(bundle->block.flags, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Blocksize */
	value = bundle->block.block_size;
	ret = sdnv_encode(value, &buffer[offs], max_len - offs);
	if (ret < 0)
		return -1;
	offs += ret;

	/* Payload */
	memcpy(&buffer[offs], MMEM_PTR(&bundle->block.payload), bundle->block.block_size);
	offs += bundle->block.block_size;

	return offs;
}

uint16_t delete_bundle(struct bundle_t *bundle)
{
	PRINTF("BUNDLE: delete %p %p %p %u\n", bundle, bundle->mem, bundle->mem.ptr,bundle->rec_time);
	if (bundle->block.block_size){
		mmem_free(&bundle->block.payload);
		bundle->block.block_size = 0;
	}
	return 1;
}

rimeaddr_t convert_eid_to_rime(uint32_t eid) {
	rimeaddr_t dest;
	dest.u8[1] = (eid & 0x000000FF) >> 0;
	dest.u8[0] = (eid & 0x0000FF00) >> 8;
	return dest;
}

uint32_t convert_rime_to_eid(rimeaddr_t * dest) {
	uint32_t eid = 0;
	eid = (dest->u8[1] & 0xFF) + ((dest->u8[0] & 0xFF) << 8);
	return eid;
}
