/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \brief Bundle Slot memory management
 *
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "logging.h"

#include "bundle.h"
#include "storage.h"
#include "agent.h"
#include "bundleslot.h"

/* Defines how many bundles can be used (in storage, used) on this node at once */
#ifdef CONF_BUNDLE_NUM
#define BUNDLE_NUM CONF_BUNDLE_NUM
#else
#define BUNDLE_NUM (BUNDLE_STORAGE_SIZE + 10)
#endif

#define INIT_GUARD() \
	do {\
		if (!inited) {\
			memset(bundleslots, 0, sizeof(bundleslots));\
			inited = 1;\
		}\
	} while (0)

static uint8_t inited = 0;
static struct bundle_slot_t bundleslots[BUNDLE_NUM];
static uint8_t slots_in_use = 0;

/* Returns a pointer to a newly allocated bundle */
struct bundle_slot_t *bundleslot_get_free()
{
	uint16_t i;
	INIT_GUARD();

	for (i=0; i<BUNDLE_NUM; i++) {
		if (bundleslots[i].ref == 0) {
			memset(&bundleslots[i], 0, sizeof(struct bundle_slot_t));

			bundleslots[i].ref++;
			bundleslots[i].type = 0;
			slots_in_use ++;

			return &bundleslots[i];
		}
	}
	return NULL;
}

/* Frees the bundle */
void bundleslot_free(struct bundle_slot_t *bs)
{
	if( bs->bundle.ptr == NULL ) {
		LOG(LOGD_DTN, LOG_SLOTS, LOGL_ERR, "DUPLICATE FREE");
		return;
	}

	bs->ref = 0;
	slots_in_use --;

	LOG(LOGD_DTN, LOG_SLOTS, LOGL_DBG, "bundleslot_free(%p) %u", bs, slots_in_use);

	// Zeroify all freed memory
	memset(bs->bundle.ptr, 0, sizeof(struct bundle_t));

	mmem_free(&bs->bundle);

	// And kill the pointer!
	bs->bundle.ptr = NULL;
}

/* Increment usage count */
int bundleslot_increment(struct bundle_slot_t *bs)
{
	LOG(LOGD_DTN, LOG_SLOTS, LOGL_DBG, "bundleslot_inc(%p) to %u", bs, bs->ref+1);

	if (!bs->ref)
		return -1;

	bs->ref++;
	return bs->ref;
}

/* Decrement usage count, free if necessary */
int bundleslot_decrement(struct bundle_slot_t *bs)
{
	LOG(LOGD_DTN, LOG_SLOTS, LOGL_DBG, "bundleslot_dec(%p) to %u", bs, bs->ref-1);

	if (!bs->ref)
		return -1;

	bs->ref--;
	if (!bs->ref)
		bundleslot_free(bs);
	return bs->ref;
}
