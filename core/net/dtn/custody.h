#ifndef __CUSTODY_H__
#define __CUSTODY_H__

#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/custody-signal.h"



struct custody_driver {
	char *name;
	
	void (* init)(void);
	uint8_t (* set_state)(custody_signal_t *signal);
	uint8_t (* manage)(custody_signal_t *signal);
	uint8_t (* decide)(custody_signal_t *signa);
};

extern const struct custody_driver CUSTODY;

#endif
