#ifndef __CUSTODY_H__
#define __CUSTODY_H__

#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/custody-signal.h"
#include "bundle.h"



struct custody_driver {
	char *name;
	
	void (* init)(void);
	uint8_t (* release)(struct bundle_t *bundle);
	uint8_t (* report)(struct bundle_t *bundle, uint8_t status);
	int32_t (* decide)(struct bundle_t *bundle);
	uint8_t (* retransmit)(struct bundle_t *bundle);
	void (* del_from_list)(uint16_t bundle_num);
};

extern const struct custody_driver CUSTODY;

#endif
