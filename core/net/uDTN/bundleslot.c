#include <stdint.h>
#include <string.h>
#include "bundleslot.h"

/* Defines how many bundles can be used (in storage, used) on this node at once */
#ifdef CONF_BUNDLE_NUM
#define BUNDLE_NUM CONF_BUNDLE_NUM
#else
#define BUNDLE_NUM 50
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

/* Returns a pointer to a newly allocated bundle */
struct bundle_slot_t *bundleslot_get_free()
{
	uint16_t i;
	INIT_GUARD();

	for (i=0; i<BUNDLE_NUM; i++) {
		if (bundleslots[i].ref == 0) {
			bundleslots[i].ref++;
			bundleslots[i].type = 0;
			return &bundleslots[i];
		}
	}
	return NULL;
}

/* Frees the bundle */
void bundleslot_free(struct bundle_slot_t *bs)
{
	bs->ref = 0;
	mmem_free(&bs->bundle);
}

/* Increment usage count */
int bundleslot_inc(struct bundle_slot_t *bs)
{
	if (!bs->ref)
		return -1;

	bs->ref++;
	return bs->ref;
}

/* Decrement usage count, free if necessary */
int bundleslot_dec(struct bundle_slot_t *bs)
{
	if (!bs->ref)
		return -1;

	bs->ref--;
	if (!bs->ref)
		bundleslot_free(bs);
	return bs->ref;
}
