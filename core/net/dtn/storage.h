#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/bundle.h"
struct storage_driver {
	char *name;

	void (* init)(void);
	void (* reinit)(void);
	int32_t (* save_bundle)(struct bundle_t *bundle);
	uint16_t (* del_bundle)(uint16_t bundle_num);
	uint16_t (* read_bundle)(uint16_t bundle_num,struct bundle_t *bundle);
	uint16_t (* free_space)(struct bundle_t *bundle);
};

extern const struct storage_driver BUNDLE_STORAGE;
#endif
