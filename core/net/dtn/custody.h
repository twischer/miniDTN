#ifndef __CUSTODY_H__
#define __CUSTODY_H__

#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/custody-signal.h"
#include "bundle.h"



struct custody_driver {
	char *name;
	
	void (* init)(void);
	uint8_t (* set_state)(custody_signal_t *signal);
	uint8_t (* manage)(struct bundle_t *bundle);
	int32_t (* decide)(struct bundle_t *bundle);
};

extern const struct custody_driver CUSTODY;

#endif
