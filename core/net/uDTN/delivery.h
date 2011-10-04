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

/**
*   \brief delivers bundle to a registrated service
*
*   \param bundle pointer to bundle 
*   \param registration pointer to registrated service
*/
void deliver_bundle(struct bundle_t *bundle, struct registration *n);

#endif

/** @} */
/** @} */
