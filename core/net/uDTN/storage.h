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

/**
 * Which storage driver are we going to use?
 * Default is RAM
 */
#ifdef CONF_BUNDLE_STORAGE
#define BUNDLE_STORAGE CONF_BUNDLE_STORAGE
#else
#define BUNDLE_STORAGE r_storage
#endif

/**
 * How many bundles can possibly be stored in the data structures?
 */
#ifdef BUNDLE_CONF_STORAGE_SIZE
#define BUNDLE_STORAGE_SIZE BUNDLE_CONF_STORAGE_SIZE
#else
#define BUNDLE_STORAGE_SIZE 	10
#endif

/**
 * How do we call the bundle list representation file (if applicable)
 */
#define BUNDLE_STORAGE_FILE_NAME "list_file"

/**
 * Representation of a bundle as returned by the "get_bundles" call to the storage module
 */
struct storage_entry_t {
	/** pointer to the next list element */
	struct storage_entry_t * next;

	/** Internal number of the bundle */
	uint32_t bundle_num;
};

/** storage module interface  */
struct storage_driver {
	char *name;
	/** called by agent a startup */
	void (* init)(void);
	void (* reinit)(void);
	/** saves a bundle */
	uint8_t (* save_bundle)(struct mmem *bundlemem, uint32_t ** bundle_number, uint8_t force);
	/** deletes a bundle */
	uint16_t (* del_bundle)(uint32_t bundle_num, uint8_t reason);
	/** reads a bundle */
	struct mmem *(* read_bundle)(uint32_t bundle_num);
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
