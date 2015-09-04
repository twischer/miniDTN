#include "convergence_layers.h"
#include "convergence_layer.h"
#include "convergence_layer_udp.h"


bool convergence_layers_init(void)
{
	if (!convergence_layer_init()) {
		return false;
	}

	if (!convergence_layer_udp_init()) {
		return false;
	}

	return true;
}
