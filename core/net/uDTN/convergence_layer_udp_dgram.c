#include "convergence_layer_udp_dgram.h"

#include "lib/logging.h"
#include "agent.h"
#include "discovery.h"


#define IP_ADDR_STRING_LENGTH	16

/* copiied from IBR-DTN:/daemon/src/net/DatagramConvergenceLayer.h */
enum HEADER_FLAGS
{
	HEADER_UNKOWN = 0,
	HEADER_BROADCAST = 1,
	HEADER_SEGMENT = 2,
	HEADER_ACK = 4,
	HEADER_NACK = 8
};


int convergence_layer_udp_dgram_send_discovery(const uint8_t* const payload, const uint8_t length)
{
	char addr_str[IP_ADDR_STRING_LENGTH];
	ipaddr_ntoa_r(&udp_mcast_addr, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Sending discovery to %s", addr_str);

	const size_t buffer_length = length + 2;
	uint8_t buffer[buffer_length];

	/* Discovery Prefix */
	buffer[0] = HEADER_BROADCAST;
	/* flags (4-bit) + seqno (4-bit) */
	buffer[1] = 0;

	/* Copy the discovery message */
	memcpy(buffer + 2, payload, length);

	/* Send it out via the MAC */
	const int ret = convergence_layer_udp_send_data(&udp_mcast_addr, buffer, buffer_length);

	const uint8_t status = (ret < 0) ? CONVERGENCE_LAYER_STATUS_NOSEND : CONVERGENCE_LAYER_STATUS_OK;
	convergence_layer_status(NULL, status);

	return 1;
}


int convergence_layer_udp_dgram_incoming_frame(const cl_addr_t* const source, const uint8_t* const payload, const size_t length, const packetbuf_attr_t rssi)
{
	// TODO use addr_t instead of cl_addr_t
	// vllt so lassen für gleiches interface für dgram:udp und dgram:lowpan

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(source, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Incoming frame from %s (header 0x%02x 0x%02x)", addr_str, payload[0], payload[1]);

	/* Notify the discovery module, that we have seen a peer */
	DISCOVERY.alive_ip(&source->ip, source->port);

	const enum HEADER_FLAGS type = payload[0];
	const int flags = (payload[1] & 0xFF) >> 4;
	const int sequence_number = (payload[1] & 0x0F) >> 0;

	const uint8_t* const data_pointer = payload + 2;
	const size_t data_length = length - 2;

	switch(type) {
	case HEADER_SEGMENT:
		/* is data */
		return convergence_layer_incoming_data(source, data_pointer, data_length, rssi, sequence_number, flags);

	case HEADER_BROADCAST:
		/* is discovery */
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Incoming discovery frame from %s", addr_str);
		DISCOVERY.receive_ip(&source->ip, data_pointer, data_length);
		return 1;

	case HEADER_ACK:
		/* is ACK */
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Incoming Ack frame from %s with SeqNo %u", addr_str, sequence_number);
		convergence_layer_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_ACK, flags);
		return 1;

	case HEADER_NACK:
		/* is NACK */
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Incoming Nack frame from %s with SeqNo %u", addr_str, sequence_number);
		convergence_layer_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_NACK, flags);
		return 1;

	default:
		return 0;
	}
}
