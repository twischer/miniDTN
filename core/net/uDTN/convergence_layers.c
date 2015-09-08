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

/**
 * @brief convergence_layers_send_discovery sends a broadcast discovery message with all
 * implemented convergence layers
 * @param payload content of the discovery message
 * @param length length of the discovery message
 * @return
 */
int convergence_layers_send_discovery(const uint8_t* const payload, const uint8_t length)
{
	int err = 0;

	static const linkaddr_t bcast_addr = {{0, 0}};
	if (!convergence_layer_send_discovery(payload, length, &bcast_addr)) {
		err = -1;
	}

	if (!convergence_layer_udp_send_discovery(payload, length)) {
		err = -2;
	}

	return err;
}
