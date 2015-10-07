#ifndef CONVERGENCE_LAYERS
#define CONVERGENCE_LAYERS

// TODO can be removed,
// if all convergence_layer_dgram calls hidden behind the convergence_layers abstraction
#include "convergence_layer.h"
#include "convergence_layer_lowpan_dgram.h"
#include "convergence_layer_udp.h"

bool convergence_layers_init(void);
int convergence_layers_send_discovery_ethernet(const uint8_t* const payload, const size_t length);

#endif // CONVERGENCE_LAYERS

