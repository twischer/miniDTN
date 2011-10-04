/**
 * \addtogroup agent
 * @{
 */

 /**
 * \defgroup redundance Redundance check moduldes
 *
 * @{
 */

/**
* \file
* defines the interface for redundance check moduldes
* \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
*/
#ifndef __REDUNDANCE_H__
#define __REDUNDANCE_H__

#include <stdlib.h>
#include <stdio.h>
#include "bundle.h"

/** interface for redundance check moduldes*/
struct redundance_check {
	char *name;
	/** called by agent at startup*/
	void (* init)(void);
	/** checks if bundel was delivered befor*/
	uint8_t (* check)(struct bundle_t *bundle);
	/** sets that bundle was deliverd */
	uint8_t (* set)(struct bundle_t *bundle);
};

extern const struct redundance_check REDUNDANCE;
/** @} */
/** @} */
#endif
