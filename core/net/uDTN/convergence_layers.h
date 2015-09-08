#ifndef CONVERGENCE_LAYERS
#define CONVERGENCE_LAYERS

// TODO can be removed,
// if all convergence_layer_pan calls hidden behind the convergence_layers abstraction
#include "convergence_layer.h"


bool convergence_layers_init(void);
int convergence_layers_send_discovery(const uint8_t* const payload, const uint8_t length);

#endif // CONVERGENCE_LAYERS

