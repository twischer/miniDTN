#ifndef __B_REDUNDANCE_H__
#define __B_REDUNDANCE_H__
#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/bundle.h"
extern const struct redundance_check b_redundance;

struct red_bundle {
	uint32_t src;
	uint32_t seq_nr;
	uint32_t fraq_offset;
	uint32_t lifetime;
	struct red_bundle *next;
};



void init(void);

void reinit(void);

uint8_t check(struct bundle_t *bundle);

uint8_t set(struct bundle_t *bundle);

#endif
