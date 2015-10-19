#ifndef CONVERGENCE_LAYER_LOWPAN_DGRAM
#define CONVERGENCE_LAYER_LOWPAN_DGRAM

#include "net/packetbuf.h"
#include "convergence_layers.h"

const struct convergence_layer clayer_lowpan_dgram;

int convergence_layer_lowpan_dgram_status(const void* const pointer, const uint8_t outcome);

#endif // CONVERGENCE_LAYER_LOWPAN_DGRAM

