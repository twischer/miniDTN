/**
 * \addtogroup agent
 * @{
 */

 /**
 * \defgroup custody Custody modules
 *
 * @{
 */

/**
 * \file 
 * this file defines the interface for custody modules
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
 */
#ifndef __CUSTODY_H__
#define __CUSTODY_H__

#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/custody-signal.h"
#include "bundle.h"

/**
 * The structure of a custody modul.
*/
struct custody_driver {
	char *name;
 	/** initializes the custody modul, called by agent at startup */
	void (* init)(void);
	/** release the bundle */
	uint8_t (* release)(struct bundle_t *bundle);
	/** sends a report to the "report to"-node with the state of the bundle */
	uint8_t (* report)(struct bundle_t *bundle, uint8_t status);
	/** decides if this node becomes custodian or not */
	int32_t (* decide)(struct bundle_t *bundle);
	/** retransmits the bundle */
	uint8_t (* retransmit)(struct bundle_t *bundle);
	/** deletes the bundle from the interal bundle list */
	void (* del_from_list)(uint16_t bundle_num);
};

extern const struct custody_driver CUSTODY;

#endif
/** @} */
/** @} */

