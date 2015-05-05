/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \brief Header for Bundle Slot memory management
 *
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 */

#ifndef __BUNDLESLOT_H__
#define __BUNDLESLOT_H__

#include <stddef.h>

#include "mmem.h"
#include "contiki.h"

/* Kernel container_of function
 * WARNING: The ({}) macro extenstion is GCC-specific,
 * but it's worth using it here */
#define container_of(ptr, type, member) ({ \
		const typeof( ((type *)0)->member ) *__mptr = (ptr); \
		(type *)( (char *)__mptr - offsetof(type,member) );})

struct bundle_slot_t {
	struct bundle_slot_t *next;
	uint8_t ref;
	int type;
	struct mmem bundle;
};

struct bundle_slot_t *bundleslot_get_free();

void bundleslot_free(struct bundle_slot_t *bs);

int bundleslot_increment(struct bundle_slot_t *bs);

int bundleslot_decrement(struct bundle_slot_t *bs);



#endif /* __BUNDLESLOT_H__ */
