#ifndef CONVERGENCE_LAYER_LOWPAN_DGRAM
#define CONVERGENCE_LAYER_LOWPAN_DGRAM

#include "net/packetbuf.h"
#include "cl_address.h"

size_t convergence_layer_lowpan_dgram_max_payload_length(void);


int convergence_layer_lowpan_dgram_send_discovery(uint8_t * payload, uint8_t length, linkaddr_t * neighbour);
int convergence_layer_lowpan_dgram_send_bundle(const cl_addr_t* const dest, const int sequence_number, const uint8_t flags,
											   const uint8_t* const payload, const size_t length, const void* const reference);
int convergence_layer_lowpan_dgram_send_ack(const cl_addr_t* const destination, const int sequence_number, const int type,
											const void* const reference);

int convergence_layer_lowpan_dgram_incoming_frame(const cl_addr_t* const source, const uint8_t* const payload,
												  const uint8_t length, const packetbuf_attr_t rssi);

#endif // CONVERGENCE_LAYER_LOWPAN_DGRAM

