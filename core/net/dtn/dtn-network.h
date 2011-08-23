/**
 * \addtogroup agent
 *
 * @{
 */
/**
* \defgroup bnet Network interface
* @{
*/

 /**
 * \file
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 */

#ifndef DTN_NETWORK_H
#define DTN_NETWORK_H

#include "contiki-conf.h"
#include "rime.h"
#include "net/rime/rimeaddr.h"
#include "bundle.h"
#include "routing.h"
extern const struct network_driver dtn_network_driver;

extern const struct mac_driver *dtn_network_mac;
process_event_t dtn_beacon_event;

/**
*   \brief sends a bundle  
* \param bundle pointer to bundel
* \param route  pointer to route sturct (contains the receivers address)
* \return 1
*/
int dtn_network_send(struct bundle_t *bundle, struct route_t *route);


#endif
/** @} */
/** @} */

