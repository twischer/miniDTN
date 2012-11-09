/**
 * \addtogroup bprocess
 * @{
 */

 /**
 * \file
 *         Headerfile for Bundle delivery
 *
 */
 
#ifndef DELIVERY_H
#define DELIVERY_H

#include "contiki.h"
#include "mmem.h"

/**
 * \brief unblocks a service that was previously delivering a bundle
 * \param bundle MMEM pointer
 */
void unblock_service(struct mmem * bundlemem);

/**
*   \brief delivers bundle to a registered service
*
*   \param bundle pointer to bundle 
*   \param registration pointer to registered service
*/
int deliver_bundle(struct mmem *bundlemem);

#endif

/** @} */
/** @} */
