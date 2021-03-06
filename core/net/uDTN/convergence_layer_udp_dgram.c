#include "convergence_layer_udp_dgram.h"

#include <lwip/ip.h>
#include <lwip/udp.h>
#include "led.h"
#include "lib/logging.h"
#include "agent.h"
#include "discovery.h"
#include "convergence_layer_dgram.h"


#define IP_ADDR_STRING_LENGTH	16

/* copiied from IBR-DTN:/daemon/src/net/DatagramConvergenceLayer.h */
typedef enum
{
	HEADER_UNKOWN = 0,
	HEADER_BROADCAST = 1,
	HEADER_SEGMENT = 2,
	HEADER_ACK = 4,
	HEADER_NACK = 8
} HEADER_TYPES;

/* copiied from IBR-DTN:/daemon/src/net/DatagramService.h */
typedef enum
{
	SEGMENT_FIRST = 0x02,
	SEGMENT_LAST = 0x01,
	SEGMENT_MIDDLE = 0x00,
	NACK_TEMPORARY = 0x04
} HEADER_FLAGS;


struct udp_dgram_hdr
{
	uint8_t type;
	uint8_t flags : 4;
	uint8_t seqno : 4;
}  __attribute__ ((packed));



static int convergence_layer_udp_dgram_init()
{
	/* needed initaliziation will be done by convergence_layer_udp_init() */
	return 1;
}


static inline int convergence_layer_udp_dgram_send(const ip_addr_t* const ip, const HEADER_TYPES type, const int sequence_number, const HEADER_FLAGS flags,
											const uint8_t* const payload, const size_t length, const void* const reference)
{
	// TODO use structure for package building
	uint8_t buffer[sizeof(struct udp_dgram_hdr)];

	/* Discovery Prefix */
	buffer[0] = type;
	/* flags (4-bit) + seqno (4-bit) */
	buffer[1] = ((flags << 4) & 0xF0) | (sequence_number & 0x0F);

	/* Send it out via the MAC */
	const int ret = convergence_layer_udp_send_data(ip, buffer, sizeof(buffer), payload, length);

	const uint8_t status = (ret < 0) ? CONVERGENCE_LAYER_STATUS_NOSEND : CONVERGENCE_LAYER_STATUS_OK;
	convergence_layer_dgram_status(reference, status);

	return 1;
}


static size_t convergence_layer_udp_dgram_max_payload_length(void)
{
	return ETH_MAX_ETH_PAYLOAD - sizeof(struct ip_hdr) - sizeof(struct udp_hdr) - sizeof(struct udp_dgram_hdr);
}


static uint8_t convergence_layer_udp_dgram_next_sequence_number(const uint8_t last_seqno)
{
	return (last_seqno + 1) % 16;
}


static int convergence_layer_udp_dgram_send_discovery(const uint8_t* const payload, const size_t length)
{
#if (UDP_DGRAM_DISCOVERY_ANNOUNCEMENT == 1)
	char addr_str[IP_ADDR_STRING_LENGTH];
	ipaddr_ntoa_r(&udp_mcast_addr, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Sending discovery to %s", addr_str);

	return convergence_layer_udp_dgram_send(&udp_mcast_addr, HEADER_BROADCAST, 0, SEGMENT_MIDDLE, payload, length, NULL);
#else
	return 0;
#endif
}


static int convergence_layer_udp_dgram_send_ack(const cl_addr_t* const dest, const int sequence_number, const int type, const void* const reference)
{
	configASSERT(dest->clayer == &clayer_udp_dgram);

	HEADER_TYPES header_type = HEADER_NACK;
	HEADER_FLAGS header_flags = 0;

	if( type == CONVERGENCE_LAYER_TYPE_ACK ) {
		// Construct the ACK
		header_type = HEADER_ACK;
	} else if( type == CONVERGENCE_LAYER_TYPE_NACK ) {
		// Construct the NACK
		header_type = HEADER_NACK;
	} else if( type == CONVERGENCE_LAYER_TYPE_TEMP_NACK ) {
		// Construct the temporary NACK
		header_type = HEADER_NACK;
		header_flags |= NACK_TEMPORARY;
	} else {
		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(dest, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Unkown ack type 0x%02x for node %s", type, addr_str);
		return -1;
	}

	// TODO use the port, too, because other nodes can use other ports
	return convergence_layer_udp_dgram_send(&dest->ip, header_type, sequence_number, header_flags, NULL, 0, reference);
}


static int convergence_layer_udp_dgram_send_bundle(const cl_addr_t* const dest, const int sequence_number, const uint8_t flags,
											const uint8_t* const payload, const size_t length, const void* const reference)
{
	configASSERT(dest->clayer == &clayer_udp_dgram);

	/* sending an package over ethernet */
	LED_On(LED_ORANGE);

	const HEADER_FLAGS header_flags = flags;
	// TODO use the port, too, because other nodes can use other ports
	const int ret =  convergence_layer_udp_dgram_send(&dest->ip, HEADER_SEGMENT, sequence_number, header_flags, payload, length, reference);

	/* package over ethernet sent */
	LED_Off(LED_ORANGE);

	return ret;
}


static int convergence_layer_udp_dgram_incoming_frame(const cl_addr_t* const source, const uint8_t* const payload, const size_t length, const packetbuf_attr_t rssi)
{
	configASSERT(source->clayer == &clayer_udp_dgram);

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(source, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Incoming frame from %s (header 0x%02x 0x%02x)", addr_str, payload[0], payload[1]);

	/* Notify the discovery module, that we have seen a peer */
	DISCOVERY.alive(source);

	const HEADER_TYPES type = payload[0];
	const HEADER_FLAGS header_flags = (payload[1] & 0xF0) >> 4;
	const int sequence_number = (payload[1] & 0x0F) >> 0;

	/* convert dgram:udp flags to dgram:lowpan flags */
	int flags = 0;
	if (header_flags & SEGMENT_FIRST) {
		flags |= CONVERGENCE_LAYER_FLAGS_FIRST;
	}
	if (header_flags & SEGMENT_LAST) {
		flags |= CONVERGENCE_LAYER_FLAGS_LAST;
	}
	if (header_flags & NACK_TEMPORARY) {
		/* overwrite, because only one type is possible */
		flags = CONVERGENCE_LAYER_FLAGS_FIRST;
	}

	const uint8_t* const data_pointer = payload + sizeof(struct udp_dgram_hdr);
	const size_t data_length = length - sizeof(struct udp_dgram_hdr);

	switch(type) {
	case HEADER_SEGMENT:
		/* is data */
		return convergence_layer_dgram_incoming_data(source, data_pointer, data_length, rssi, sequence_number, flags);

	case HEADER_BROADCAST:
		/* is discovery */
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Incoming discovery frame from %s", addr_str);
		DISCOVERY.receive(source, data_pointer, data_length);
		return 1;

	case HEADER_ACK:
		/* is ACK */
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Incoming Ack frame from %s with SeqNo %u", addr_str, sequence_number);
		convergence_layer_dgram_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_ACK, flags);
		return 1;

	case HEADER_NACK:
		/* is NACK */
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Incoming Nack frame from %s with SeqNo %u", addr_str, sequence_number);
		convergence_layer_dgram_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_NACK, flags);
		return 1;

	default:
		return 0;
	}
}


const struct convergence_layer clayer_udp_dgram = {
	.name = "dgram:udp",
	.init = convergence_layer_udp_dgram_init,
	.max_payload_length = convergence_layer_udp_dgram_max_payload_length,
	.next_seqno = convergence_layer_udp_dgram_next_sequence_number,
	.send_discovery = convergence_layer_udp_dgram_send_discovery,
	.send_ack = convergence_layer_udp_dgram_send_ack,
	.send_bundle = convergence_layer_udp_dgram_send_bundle,
	.input = convergence_layer_udp_dgram_incoming_frame
};
