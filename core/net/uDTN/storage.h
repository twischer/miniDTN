/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup bundle_storage Bundle Storage modules
 *
 * @{
 */

/**
 * \file 
 * \brief this file defines the interface for storage modules
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"
#include "memb.h"

#include "bundle.h"

/**
 * Which storage driver are we going to use?
 * Default is RAM
 */
#ifdef BUNDLE_CONF_STORAGE
#define BUNDLE_STORAGE BUNDLE_CONF_STORAGE
#else
#define BUNDLE_STORAGE storage_mmem
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
 * Should storage go into an initial safe state when starting up?
 * Otherwise, some storages may try to reconstruct the last start before powering down
 */
#ifdef BUNDLE_CONF_STORAGE_INIT
#define BUNDLE_STORAGE_INIT BUNDLE_CONF_STORAGE_INIT
#else
#define BUNDLE_STORAGE_INIT 0
#endif

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
	uint8_t (* save_bundle)(struct mmem *bundlemem, uint32_t ** bundle_number);
	/** deletes a bundle */
	uint16_t (* del_bundle)(uint32_t bundle_num, uint8_t reason);
	/** reads a bundle */
	struct mmem *(* read_bundle)(uint32_t bundle_num);
	/** Marks a bundle as locked so it will not be deleted until it is unlocked */
	uint8_t (* lock_bundle)(uint32_t bundle_num);
	/** Unlocks a bundle so it can be deleted again */
	void (* unlock_bundle)(uint32_t bundle_num);
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
