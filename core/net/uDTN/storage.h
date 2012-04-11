/**
 * \addtogroup agent
 * @{
 */

 /**
 * \defgroup bstorage Storage modules
 *
 * @{
 */

/**
 * \file 
 * this file defines the interface for storage modules
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
 */
#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdlib.h>
#include <stdio.h>
#include "bundle.h"
#include "contiki.h"
#include "bundle.h"
#include "memb.h"

//static uint16_t saved_as_num;

struct saved_as_t{
	uint16_t saved_as_num;
};

extern struct memb *saved_as_mem;
/** storage module interface  */
struct storage_driver {
	char *name;
	/** called by agent a startup */
	void (* init)(void);
	void (* reinit)(void);
	/** saves a bundle */
	int32_t (* save_bundle)(struct bundle_t *bundle);
	/** deletes a bundle */
	uint16_t (* del_bundle)(uint16_t bundle_num,uint8_t reason);
	/** reads a bundle */
	uint16_t (* read_bundle)(uint16_t bundle_num,struct bundle_t *bundle);
	/** checks if there is space for a bundle */
	uint16_t (* free_space)(struct bundle_t *bundle);
	/** returns the number of saved bundles */
	uint16_t (* get_bundle_num)(void);
};
extern const struct storage_driver BUNDLE_STORAGE;
#endif
/** @} */
/** @} */
