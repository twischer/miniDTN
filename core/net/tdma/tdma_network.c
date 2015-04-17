/**
 * \addtogroup tdma_network
 * @{
 */

/**
 * \file
 *
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */


#include "netstack.h"
#include "packetbuf.h"
#include "leds.h"


#include "tdma_network.h"

/**
 * Init function called by the Contiki netstack
 */
static void tdma_network_init(void) 
{
	/* Clear the packet buffer */
	packetbuf_clear();


}

/**
 * Input callback called by the lower layers to indicate incoming data
 */
static void tdma_network_input(void) 
{
	linkaddr_t source;
	uint8_t * buffer = NULL;
	uint8_t length = 0;
	packetbuf_attr_t rssi = 0;

	printf("NETWORK: got packet\n");

	/* Create a copy here, because otherwise packetbuf_clear will evaporate the address */
	linkaddr_copy(&source, packetbuf_addr(PACKETBUF_ADDR_SENDER));
	buffer = packetbuf_dataptr();
	length = packetbuf_datalen();
	rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);


	//leds_off(LEDS_ALL);
}

/**
 * Callback function called by the lower layers after a transmission attempt on the radio
 */
static void tdma_network_sent(void * pointer, int status, int num_tx)
{
	int outcome = 0;

	/* Here we map the return values to the three interesting values needed by the CL */
	switch(status) {
	case MAC_TX_DEFERRED:
	case MAC_TX_COLLISION:
	case MAC_TX_ERR:
	default:
		outcome = 0;
		break;
	case MAC_TX_ERR_FATAL:
		/* Fatal errors occur when the buffer is too small */
		outcome = 1;
		break;
	case MAC_TX_NOACK:
		outcome = 2;
		break;
	case MAC_TX_OK:
		outcome = 3;
		break;
	}

	/* Call the CL */
}

uint8_t * tdma_network_get_buffer()
{
	uint8_t * buffer = NULL;

	/* We're not going to use packetbuf_copyfrom here but instead assemble the packet
	 * in the buffer ourself */
	packetbuf_clear();

	buffer = packetbuf_dataptr();

	return buffer;
}

uint8_t tdma_network_get_buffer_length() {
	return PACKETBUF_SIZE;
}

void tdma_network_send(linkaddr_t * destination, uint8_t length, void * reference)
{
	//leds_on(LEDS_YELLOW);

	/* Set the data length */
	packetbuf_set_datalen(length);

	/* Set destination address */
	/* printf("network: %u.%u\n",destination->u8[0],destination->u8[1]); */
	packetbuf_copyfrom(reference, length);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, destination);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
	printf("network: %u.%u\n",packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0],packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[1]);
	/* Send it out via the MAC */
	NETSTACK_MAC.send(&tdma_network_sent, reference);

	//leds_off(LEDS_YELLOW);
}

/**
 * Contiki's network driver interface
 */
const struct network_driver tdma_network_driver = 
{
		"TDMA",
		tdma_network_init,
		tdma_network_input
};

/** @} */
