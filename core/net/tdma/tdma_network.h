/**
 * \addtogroup agent
 *
 * @{
 */

/**
 * \defgroup tdma_network Network Layer interface
 * @{
 */

/**
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef TDMA_NETWORK_H
#define TDMA_NETWORK_H

#include "contiki.h"
#include "contiki-conf.h"
#include "net/linkaddr.h"


extern const struct network_driver tdma_network_driver;

extern const struct mac_driver *tdma_network_mac;
process_event_t tdma_beacon_event;

/**
 * \brief Send out the content that was put in the buffer.
 * \param destination Pointer to the destination address
 * \param length Length of the outgoing frame
 * \param reference Reference that will be passed on into the callback
 */
void tdma_network_send(linkaddr_t * destination, uint8_t length, void * reference);

/**
 * \brief Returns the pointer to a buffer that can be used to contruct packets
 * \returns Pointer to the buffer
 */
uint8_t * tdma_network_get_buffer();

/**
 * \brief Returns the maximum buffer length
 * \returns Maximum buffer length
 */
uint8_t tdma_network_get_buffer_length();

#endif
/** @} */
/** @} */

