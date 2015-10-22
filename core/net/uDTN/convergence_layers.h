#ifndef CONVERGENCE_LAYERS
#define CONVERGENCE_LAYERS

#include "net/packetbuf.h"
#include "cl_address.h"


/**
 * Specify the maximum bundle size that we guarantee to support
 * Memory is reserved to serialize a bundle of this size
 */
#ifdef CONVERGENCE_LAYER_CONF_MAX_SIZE
#define CONVERGENCE_LAYER_MAX_SIZE CONVERGENCE_LAYER_CONF_MAX_SIZE
#else
#define CONVERGENCE_LAYER_MAX_SIZE ETH_MAX_ETH_PAYLOAD
#endif


struct convergence_layer {
	const char* const name;

	int (* const init)(void);

	size_t (* const max_payload_length)(void);

	uint8_t (* const next_seqno)(const uint8_t last_seqno);

	int (* const send_discovery)(const uint8_t* const payload, const size_t length);

	int (* const send_ack)(const cl_addr_t* const dest, const int seqno, const int type, const void* const reference);

	int (* const send_bundle)(const cl_addr_t* const dest, const int seqno, const uint8_t flags,
						const uint8_t* const payload, const size_t length, const void* const reference);

	int (* const input)(const cl_addr_t* const source, const uint8_t* const payload, const size_t length, const packetbuf_attr_t rssi);
};



bool convergence_layers_init(void);
int convergence_layers_send_discovery_ethernet(const uint8_t* const payload, const size_t length);

#endif // CONVERGENCE_LAYERS

