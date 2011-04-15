/**
 * \defgroup interface das Network-Layer-Interface
 *
 * @{
 */
 /**
 * \file
 *         Headerfile für das Netzwerk-Interface
 *
 */

#ifndef DTN_NETWORK_H
#define DTN_NETWORK_H

#include "contiki-conf.h"
#include "rime.h"

/**
*   \brief Treiber für das Protokoll
*
*   Notwendig für die Anbindung an den Netstack
*
*/
extern const struct network_driver dtn_network_driver;

extern const struct mac_driver *dtn_network_mac;


/**
*   \brief Sendet ein Bündel
*
*    Ein Bündel muss ich Bundlebuffer stehen um gesendet zu werden
*
*/
int dtn_network_send(uint8_t *payload_ptr, uint8_t payload_len);

/**
*	\brief send node discovery
*/
int dtn_discover(void);
#endif
/** @} */

