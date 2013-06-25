/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup redundancy Redundancy check moduldes
 *
 * @{
 */

/**
 * \file
 * \brief defines the interface for redundancy check modules
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef __REDUNDANCE_H__
#define __REDUNDANCE_H__

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"

#include "bundle.h"

/**
 * Which redundance driver are we going to use?
 */
#ifdef CONF_REDUNDANCE
#define REDUNDANCE CONF_REDUNDANCE
#else
#define REDUNDANCE redundancy_basic
#endif

/**
 * How many recent bundles should be stored in the redundance module?
 */
#define REDUNDANCE_MAX 10

/** interface for redundance check modules */
struct redundance_check {
	char *name;
	/** called by agent at startup*/
	void (* init)(void);
	/** checks if bundle was delivered before */
	uint8_t (* check)(uint32_t bundle_number);
	/** sets that bundle was delivered */
	uint8_t (* set)(uint32_t bundle_number);
};

extern const struct redundance_check REDUNDANCE;
/** @} */
/** @} */
#endif
