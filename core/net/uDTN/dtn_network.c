/**
 * \addtogroup bnet
 * @{
 */

/**
 * \file
 *
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 */

#include "netstack.h"
#include "packetbuf.h"
#include "leds.h"
#include "logging.h"

#include "convergence_layer.h"
#include "agent.h"

#include "dtn_network.h"

/**
 * Init function called by the Contiki netstack
 */
static void dtn_network_init(void) 
{
	/* Set up log domains */
	logging_init();
	logging_domain_level_set(LOGD_DTN, LOG_NET, LOGL_DBG);
	logging_domain_level_set(LOGD_DTN, LOG_BUNDLE, LOGL_DBG);
	logging_domain_level_set(LOGD_DTN, LOG_ROUTE, LOGL_DBG);
	logging_domain_level_set(LOGD_DTN, LOG_STORE, LOGL_DBG);
	logging_domain_level_set(LOGD_DTN, LOG_SDNV, LOGL_DBG);
	logging_domain_level_set(LOGD_DTN, LOG_SLOTS, LOGL_DBG);
	logging_domain_level_set(LOGD_DTN, LOG_AGENT, LOGL_DBG);
	logging_domain_level_set(LOGD_DTN, LOG_CL, LOGL_DBG);

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
	rimeaddr_t source;
	uint8_t * buffer = NULL;
	uint8_t length = 0;

	leds_on(LEDS_ALL);

	/* Create a copy here, because otherwise packetbuf_clear will evaporate the address */
	rimeaddr_copy(&source, packetbuf_addr(PACKETBUF_ADDR_SENDER));
	buffer = packetbuf_dataptr();
	length = packetbuf_datalen();

	convergence_layer_incoming_frame(&source, buffer, length);

	leds_off(LEDS_ALL);
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
	case MAC_TX_ERR_FATAL:
	default:
		outcome = CONVERGENCE_LAYER_STATUS_NOSEND;
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

void dtn_network_send(rimeaddr_t * destination, uint8_t length, void * reference)
{
	leds_on(LEDS_YELLOW);

	/* Set the data length */
	packetbuf_set_datalen(length);

	/* Set destination address */
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, destination);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);

	/* Send it out via the MAC */
	NETSTACK_MAC.send(&dtn_network_sent, reference);

	leds_off(LEDS_YELLOW);
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
