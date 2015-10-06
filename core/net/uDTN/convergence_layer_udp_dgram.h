#ifndef CONVERGENCE_LAYER_UDP_DGRAM_H
#define CONVERGENCE_LAYER_UDP_DGRAM_H

#include "convergence_layer.h"
#include "convergence_layer_udp.h"


int convergence_layer_udp_dgram_send_discovery(const uint8_t* const payload, const size_t length);
int convergence_layer_udp_dgram_send_ack(const cl_addr_t* const dest, const int sequence_number, const int type, const void* const reference);
int convergence_layer_udp_dgram_send_bundle(const cl_addr_t* const dest, const int sequence_number, const uint8_t flags,
											const uint8_t* const payload, const size_t length, const void* const reference);
int convergence_layer_udp_dgram_incoming_frame(const cl_addr_t* const source, const uint8_t* const payload, const size_t length, const packetbuf_attr_t rssi);

#endif // CONVERGENCE_LAYER_UDP_DGRAM_H

