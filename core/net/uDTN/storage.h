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

struct storage_entry_t {
	struct storage_entry_t * next;

	uint16_t bundle_num;
};

/** storage module interface  */
struct storage_driver {
	char *name;
	/** called by agent a startup */
	void (* init)(void);
	void (* reinit)(void);
	/** saves a bundle */
	int32_t (* save_bundle)(struct mmem *bundlemem);
	/** deletes a bundle */
	uint16_t (* del_bundle)(uint16_t bundle_num,uint8_t reason);
	/** reads a bundle */
	struct mmem *(* read_bundle)(uint16_t bundle_num);
	/** checks if there is space for a bundle */
	uint16_t (* free_space)(struct mmem *bundlemem);
	/** returns the number of saved bundles */
	uint16_t (* get_bundle_num)(void);
	/** returns pointer to list of bundles */
	struct storage_entry_t * (* get_bundles)(void);
};
extern const struct storage_driver BUNDLE_STORAGE;
#endif
/** @} */
/** @} */
