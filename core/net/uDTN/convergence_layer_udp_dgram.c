#include "convergence_layer_udp_dgram.h"

#include "lib/logging.h"
#include "agent.h"


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
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Sending discovery to %s", addr_str);

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
