/**
 * \addtogroup bnet
 * @{
 */

/**
 * \file
 *
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "dtn-network.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/rime/rimeaddr.h"
#include "bundle.h"
#include "agent.h"
#include "dispatching.h"
#include "routing.h"
#include "sdnv.h"
#include "mmem.h"
#include "discovery.h"
#include "stimer.h"
#include "leds.h"

//#define ENABLE_LOGGING 1
#include "logging.h"

#if CONTIKI_TARGET_AVR_RAVEN
	#include <stings.h>
#endif

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
	uint8_t *input_packet;
	int size = packetbuf_datalen();
	struct mmem *bundlemem;
	struct bundle_t *bundle;
	input_packet = packetbuf_dataptr();
	rimeaddr_t bsrc = *packetbuf_addr(PACKETBUF_ADDR_SENDER);
	packetbuf_clear();

	if((*input_packet==0x08) & (*(input_packet+1)==0x80)) {
		// Skip the first two bytes
		uint8_t * discovery_data = input_packet + 2;
		uint8_t discovery_length = (uint8_t) (size - 2);

		LOG(LOGD_DTN, LOG_NET, LOGL_DBG, "Discovery received from %u.%u", bsrc.u8[0], bsrc.u8[1]);

		leds_on(LEDS_ALL);

		DISCOVERY.receive(&bsrc, discovery_data, discovery_length);

		leds_off(LEDS_ALL);
		return;
	}

	// Skip the first byte
	uint8_t * payload_data = input_packet + 1;
	uint8_t payload_length = (uint8_t) (size - 1);

	leds_on(LEDS_GREEN);

	// packetbuf_clear();
	LOG(LOGD_DTN, LOG_NET, LOGL_DBG, "Bundle received %p from %u.%u", payload_data, bsrc.u8[0], bsrc.u8[1]);

	// Allocate memory, parse the bundle and set reference counter to 1
	bundlemem = recover_bundle(payload_data, payload_length);
	if (!bundlemem) {
		LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "Error recovering bundle");
		leds_off(LEDS_GREEN);
		return;
	}

	bundle = (struct bundle_t*) MMEM_PTR(bundlemem);

	rimeaddr_copy(&bundle->msrc, &bsrc);

#if DEBUG
	uint8_t i;
	printf("NETWORK: input ");
	/* FIXME: DEBUGGING broken */
	printf("\n");
#endif

	// Notify the discovery module, that we have seen a peer
	DISCOVERY.alive(&bsrc);
	LOG(LOGD_DTN, LOG_NET, LOGL_DBG, "size of received bundle: %u pointer %p", bundlemem->size, bundle);

	dispatch_bundle(bundlemem);

	leds_off(LEDS_GREEN);
}

/**
 * Callback function called by the lower layers after a transmission attempt on the radio
 */
static void packet_sent(void *ptr, int status, int num_tx) 
{
	switch(status) {
	  case MAC_TX_COLLISION:
	    LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "collision after %d tx", num_tx);
	    break;
	  case MAC_TX_NOACK:
	    LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "noack after %d tx", num_tx);
	    break;
	  case MAC_TX_OK:
	    LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "sent after %d tx", num_tx);
	    break;
	  case MAC_TX_DEFERRED:
	    LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "error MAC_TX_DEFERRED after %d tx", num_tx);
	    break;
	  case MAC_TX_ERR:
	    LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "error MAC_TX_ERR after %d tx", num_tx);
	    break;
	  case MAC_TX_ERR_FATAL:
	    LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "error MAC_TX_ERR_FATAL after %d tx", num_tx);
	    break;
	  default:
	    LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "error %d after %d tx", status, num_tx);
	    break;
	  }

	if( !ptr ){
		LOG(LOGD_DTN, LOG_NET, LOGL_WRN, "callback with empty pointer");
		return;
	}

	struct route_t * route= (struct route_t *) ptr;
	LOG(LOGD_DTN, LOG_NET, LOGL_DBG, "MAC Callback for bundle_num %u with ptr %p",route->bundle_num,ptr);

	if( status == MAC_TX_OK ) {
		// Notify discovery that peer is still alive
		DISCOVERY.alive(&route->dest);
	}

	ROUTING.sent(route, status, num_tx);
}

/**
 * Internal function called by the agent to transmit bundles
 */
int dtn_network_send(struct mmem * bundlemem, struct route_t * route)
{
	uint8_t * buffer = NULL;
	uint8_t len;

	leds_on(LEDS_YELLOW);

	LOG(LOGD_DTN, LOG_NET, LOGL_DBG, "send %lu (%p) via %d.%d", route->bundle_num, bundlemem, route->dest.u8[0], route->dest.u8[1]);

	/* We're not going to use packetbuf_copyfrom here but instead assemble the packet
	 * in the buffer ourself */
	packetbuf_clear();

	buffer = packetbuf_dataptr();

	//Extended buffer byte
	buffer[0] = 0x30;

	len = encode_bundle(bundlemem, buffer+1, PACKETBUF_SIZE-1);
	packetbuf_set_datalen(len+1);

	/* Set destination address */
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &route->dest);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);

	/* Send it out via the MAC */
	NETSTACK_MAC.send(&packet_sent, route);
	
	leds_off(LEDS_YELLOW);

	return 1;
}

/**
 * Internal function called by the discovery module to transmit outgoing discovery beacons
 */
int dtn_send_discover(uint8_t *payload, uint8_t len, rimeaddr_t *dst)
{
	uint8_t * buffer = NULL;

	/* We're not going to use packetbuf_copyfrom here but instead assemble the packet
	 * in the buffer ourself */
	packetbuf_clear();

	buffer = packetbuf_dataptr();

	// Discovery Prefix
	buffer[0] = 0x08;
	buffer[1] = 0x80;

	// Copy the discovery message and set the length
	memcpy(&buffer[2], payload, len);
	packetbuf_set_datalen(len+2);

	/* Set destination address */
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, dst);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);

	/* Send it out via the MAC */
	NETSTACK_MAC.send(NULL, NULL); 

	return 1;
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
