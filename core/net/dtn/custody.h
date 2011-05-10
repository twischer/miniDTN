#ifndef __CUSTODY_H__
#define __CUSTODY_H__

#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/bundle.h"


struct custody_driver {
	char *name;
	
	void (* init)(void);
	uint8_t (* set_state)(uint8_t state, uint8_t reason, struct bundle_t *bundle);
	uint8_t (* manage)(struct bundle_t *bundle);
	uint8_t (* decide)(struct bundle_t *bundle);
};

extern const struct custody_driver COSUTODY;

#endif
