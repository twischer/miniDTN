#ifndef __B_REDUNDANCE_H__
#define __B_REDUNDANCE_H__
#include "net/dtn/bundle.h"

#define B_RED_MAX 10

extern const struct redundance_check b_redundance_check;

struct red_bundle_t {
	uint8_t buf;
	uint32_t src;
	uint32_t seq_nr;
	uint32_t frag_offset;
	uint32_t lifetime;
	struct  red_bundle_t *next;
};


uint8_t check(struct bundle_t *bundle);

uint8_t set(struct bundle_t *bundle);

#endif
