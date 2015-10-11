#include "convergence_layers.h"
#include "convergence_layer_dgram.h"
#include "convergence_layer_udp.h"
#include "convergence_layer_udp_dgram.h"


bool convergence_layers_init(void)
{
	if (!convergence_layer_dgram_init()) {
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
int convergence_layers_send_discovery_ethernet(const uint8_t* const payload, const size_t length)
{
	int err = 0;

#if (UDP_DGRAM_DISCOVERY_ANNOUNCEMENT == 1)
	if (convergence_layer_udp_dgram_send_discovery(payload, length) < 0) {
		err += -1;
	}
#endif

#ifdef UDP_DISCOVERY_ANNOUNCEMENT
	if (convergence_layer_udp_send_discovery(payload, length) < 0) {
		err += -2;
	}
#endif /* UDP_DISCOVERY_ANNOUNCEMENT */

	return err;
}
