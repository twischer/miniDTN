/**
 * \addtogroup agent
 *
 * @{
 */

/**
 * \defgroup dtn_network Network Layer interface
 * @{
 */

/**
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef DTN_NETWORK_H
#define DTN_NETWORK_H

#include "contiki-conf.h"
#include "net/rime/rime.h"
#include "net/linkaddr.h"

#include "bundle.h"
#include "routing.h"

extern const struct network_driver dtn_network_driver;

extern const struct mac_driver *dtn_network_mac;

/**
 * \brief Send out the content that was put in the buffer.
 * \param destination Pointer to the destination address
 * \param length Length of the outgoing frame
 * \param reference Reference that will be passed on into the callback
 */
void dtn_network_send(linkaddr_t * destination, uint8_t length, void * reference);

/**
 * \brief Returns the pointer to a buffer that can be used to contruct packets
 * \returns Pointer to the buffer
 */
uint8_t * dtn_network_get_buffer();

/**
 * \brief Returns the maximum buffer length
 * \returns Maximum buffer length
 */
uint8_t dtn_network_get_buffer_length();

#endif
/** @} */
/** @} */

