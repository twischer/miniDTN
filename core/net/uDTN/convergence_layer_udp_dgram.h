#ifndef CONVERGENCE_LAYER_UDP_DGRAM_H
#define CONVERGENCE_LAYER_UDP_DGRAM_H

#include "net/packetbuf.h"
#include "cl_address.h"
#include "convergence_layer_udp.h"

#define UDP_DGRAM_DISCOVERY_ANNOUNCEMENT	0


size_t convergence_layer_udp_dgram_max_payload_length(void);

int convergence_layer_udp_dgram_send_ack(const cl_addr_t* const dest, const int sequence_number, const int type, const void* const reference);
int convergence_layer_udp_dgram_send_bundle(const cl_addr_t* const dest, const int sequence_number, const uint8_t flags,
											const uint8_t* const payload, const size_t length, const void* const reference);

int convergence_layer_udp_dgram_incoming_frame(const cl_addr_t* const source, const uint8_t* const payload, const size_t length, const packetbuf_attr_t rssi);

#if (UDP_DGRAM_DISCOVERY_ANNOUNCEMENT == 1)
int convergence_layer_udp_dgram_send_discovery(const uint8_t* const payload, const size_t length);
#endif

#endif // CONVERGENCE_LAYER_UDP_DGRAM_H
