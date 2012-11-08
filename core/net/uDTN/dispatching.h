/**
 * \addtogroup bprocess
 * @{
 */
 
 
 /**
 * \file
 *         
 */
#ifndef DISPATCHING_H
#define DISPATCHING_H

/**
*   \brief decides if bundle must be delivered or forwarded
*
*   \param  bundle bundle to be processed
*   \returns <= 0 on error >0 on success
*/
int dispatch_bundle(struct mmem *bundlemem);

#endif

/** @} */
/** @} */
