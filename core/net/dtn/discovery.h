/**
 * \addtogroup agent
 * @{
 */

 /**
 * \defgroup discovery Discovery modules
 *
 * @{
 */

/**
* \file
* defines the interface for discovery modules
* \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
*/
#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <stdint.h>
#include "net/dtn/bundle.h"
#include "mmem.h"

/** interface for discovery modules */
struct discovery_driver {
	char *name;
	/** sends discovery message */
	void (* send)(uint16_t num);
	/** return 1 if msg is a discovery answer*/
	uint8_t (* is_beacon)(uint8_t *msg);
	/** return 1 if msg is a discovery message */
	uint8_t (* is_discover)(uint8_t *msg);
};

extern const struct discovery_driver DISCOVERY;

#endif
/** @} */
/** @} */
