/**
* \file
*
*/
#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <stdint.h>
#include "net/dtn/bundle.h"
#include "mmem.h"
struct discovery_driver {
	char *name;
	void (* send)(void);
	uint8_t (* is_beacon)(uint8_t *msg);
	uint8_t (* is_discover)(uint8_t *msg);
};

extern const struct discovery_driver DISCOVERY;

#endif
