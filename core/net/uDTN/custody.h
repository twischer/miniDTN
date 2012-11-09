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

#include "mmem.h"
#include "contiki.h"

/**
 * Which custody driver are we going to use?
 */
#ifdef CONF_CUSTODY
#define CUSTODY CONF_CUSTODY
#else
#define CUSTODY custody_basic
#endif

/**
 * The structure of a custody modul.
*/
struct custody_driver {
	char *name;
 	/** initializes the custody modul, called by agent at startup */
	void (* init)(void);
	/** release the bundle */
	uint8_t (* release)(struct mmem *bundlemem);
	/** sends a report to the "report to"-node with the state of the bundle */
	uint8_t (* report)(struct mmem *bundlemem, uint8_t status);
	/** decides if this node becomes custodian or not */
	uint8_t (* decide)(struct mmem *bundlemem, uint32_t * bundle_number);
	/** retransmits the bundle */
	uint8_t (* retransmit)(struct mmem *bundlemem);
	/** deletes the bundle from the interal bundle list */
	void (* del_from_list)(uint32_t bundle_number);
};

extern const struct custody_driver CUSTODY;

#endif
/** @} */
/** @} */

