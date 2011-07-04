#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/bundle.h"
#include "contiki.h"
#include "bundle.h"

process_event_t dtn_bundle_deleted_event;

static uint16_t saved_as_num;
extern uint16_t del_num;


struct storage_driver {
	char *name;

	void (* init)(void);
	void (* reinit)(void);
	int32_t (* save_bundle)(struct bundle_t *bundle);
	uint16_t (* del_bundle)(uint16_t bundle_num,uint8_t reason);
	uint16_t (* read_bundle)(uint16_t bundle_num,struct bundle_t *bundle);
	uint16_t (* free_space)(struct bundle_t *bundle);
	uint16_t (* get_bundle_num)(void);
};
extern const struct storage_driver BUNDLE_STORAGE;
#endif
