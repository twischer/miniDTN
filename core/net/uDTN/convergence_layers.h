#ifndef CONVERGENCE_LAYERS
#define CONVERGENCE_LAYERS

// TODO can be removed,
// if all convergence_layer_dgram calls hidden behind the convergence_layers abstraction
#include "convergence_layer_dgram.h"
#include "convergence_layer_lowpan_dgram.h"
#include "convergence_layer_udp.h"

/**
 * Specify the maximum bundle size that we guarantee to support
 * Memory is reserved to serialize a bundle of this size
 */
#ifdef CONVERGENCE_LAYER_CONF_MAX_SIZE
#define CONVERGENCE_LAYER_MAX_SIZE CONVERGENCE_LAYER_CONF_MAX_SIZE
#else
#define CONVERGENCE_LAYER_MAX_SIZE ETH_MAX_ETH_PAYLOAD
#endif


bool convergence_layers_init(void);
int convergence_layers_send_discovery_ethernet(const uint8_t* const payload, const size_t length);

#endif // CONVERGENCE_LAYERS

