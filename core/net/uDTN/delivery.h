/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \brief Headerfile for Bundle delivery
 *
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */
 
#ifndef DELIVERY_H
#define DELIVERY_H

#include "contiki.h"
#include "mmem.h"

/**
 * \brief unblocks a service that was previously delivering a bundle
 * \param bundlemem Pointer to MMEM structure
 */
void delivery_unblock_service(struct mmem * bundlemem);

/**
*   \brief delivers bundle to a registered service
*
*   \param bundlemem Pointer to MMEM structure
*/
int delivery_deliver_bundle(struct mmem * bundlemem);

#endif

/** @} */
/** @} */
