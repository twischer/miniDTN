/**
 * \addtogroup dtn_network
 * @{
 */

/**
 * \file
 *
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Pöttner <poettner@ibr.cs.tu-bs.de>
 */

#ifdef CONF_LOGLEVEL
#define LOGLEVEL CONF_LOGLEVEL
#else
#define LOGLEVEL LOGL_DBG
#endif

#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/framer-802154.h"
#include "dev/leds.h"
#include "lib/logging.h"

#include "convergence_layer.h"
#include "agent.h"
#include "rf230bb.h"

#include "dtn_network.h"

/**
 * Init function called by the Contiki netstack
 */
static void dtn_network_init(void) 
{
	/* Set up log domains */
	logging_init();
	logging_domain_level_set(LOGD_DTN, LOG_NET, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_BUNDLE, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_ROUTE, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_STORE, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_SDNV, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_SLOTS, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_AGENT, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_CL, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_CL_UDP, LOGLEVEL);
	logging_domain_level_set(LOGD_DTN, LOG_DISCOVERY, LOGLEVEL);

	/* Clear the packet buffer */
	packetbuf_clear();

	/* Initialize logging */
	LOG(LOGD_DTN, LOG_NET, LOGL_DBG, "init");

	/* Start the agent */
	agent_init();
}

/**
 * Input callback called by the lower layers to indicate incoming data
 */
static void dtn_network_input(void) 
{
	linkaddr_t source;
	uint8_t * buffer = NULL;
	uint8_t length = 0;
	packetbuf_attr_t rssi = 0;

//	leds_on(LEDS_ALL);

	/* Create a copy here, because otherwise packetbuf_clear will evaporate the address */
	linkaddr_copy(&source, packetbuf_addr(PACKETBUF_ADDR_SENDER));
	buffer = packetbuf_dataptr();
	length = packetbuf_datalen();
	rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

	convergence_layer_incoming_frame(&source, buffer, length, rssi);

//	leds_off(LEDS_ALL);
}

/**
 * Callback function called by the lower layers after a transmission attempt on the radio
 */
static void dtn_network_sent(void * pointer, int status, int num_tx)
{
	int outcome = 0;

	/* Here we map the return values to the three interesting values needed by the CL */
	switch(status) {
	case MAC_TX_DEFERRED:
	case MAC_TX_COLLISION:
	case MAC_TX_ERR:
	default:
		outcome = CONVERGENCE_LAYER_STATUS_NOSEND;
		break;
	case MAC_TX_ERR_FATAL:
		/* Fatal errors occur when the buffer is too small */
		outcome = CONVERGENCE_LAYER_STATUS_FATAL;
		break;
	case MAC_TX_NOACK:
		outcome = CONVERGENCE_LAYER_STATUS_NOACK;
		break;
	case MAC_TX_OK:
		outcome = CONVERGENCE_LAYER_STATUS_OK;
		break;
	}

	/* Call the CL */
	convergence_layer_status(pointer, outcome);
}

uint8_t * dtn_network_get_buffer()
{
	uint8_t * buffer = NULL;

	/* We're not going to use packetbuf_copyfrom here but instead assemble the packet
	 * in the buffer ourself */
	packetbuf_clear();

	buffer = packetbuf_dataptr();

	return buffer;
}

uint8_t dtn_network_get_buffer_length() {
	return PACKETBUF_SIZE;
}

void dtn_network_send(linkaddr_t * destination, uint8_t length, void * reference)
{
//	leds_on(LEDS_YELLOW);

	/* Set the data length */
	packetbuf_set_datalen(length);

	/* Set destination address */
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, destination);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
	/* Make sure we always send ieee802.15.4 data frames*/
	packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);

	/* Send it out via the MAC */
	nullrdc_driver.send(&dtn_network_sent, reference);

//	leds_off(LEDS_YELLOW);
}

/**
 * Contiki's network driver interface
 */
const struct network_driver dtn_network_driver = 
{
		"DTN",
		dtn_network_init,
		dtn_network_input
};

/** @} */
