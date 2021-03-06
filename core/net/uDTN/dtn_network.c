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
#include "lib/logging.h"

#include "convergence_layer_lowpan_dgram.h"
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
	/* Create a copy here, because otherwise packetbuf_clear will evaporate the address */
	cl_addr_t source;
	cl_addr_build_lowpan_dgram(packetbuf_addr(PACKETBUF_ADDR_SENDER), &source);

	const uint8_t* const buffer = packetbuf_dataptr();
	const uint8_t length = packetbuf_datalen();
	const packetbuf_attr_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

	source.clayer->input(&source, buffer, length, rssi);
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

	// TODO add parameter to dtn_network_sent to set callback by calling
	/* Call the CL */
	convergence_layer_lowpan_dgram_status(pointer, outcome);
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

int dtn_network_send(linkaddr_t * destination, uint8_t length, void * reference)
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
	return nullrdc_driver.send(&dtn_network_sent, reference);

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
