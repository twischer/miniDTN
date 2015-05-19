/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \brief Bundle dispatching module
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef DISPATCHING_H
#define DISPATCHING_H

#include "lib/mmem.h"

/**
*   \brief Handles Admin Records, custody bundles and regular bundles
*
*   \param bundlemem Pointer to MMEM structure
*   \returns <= 0 on error >0 on success
*/
int dispatching_dispatch_bundle(struct mmem * bundlemem);

#endif

/** @} */
/** @} */
