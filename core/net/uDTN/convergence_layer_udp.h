#ifndef CONVERGENCE_LAYER_UDP
#define CONVERGENCE_LAYER_UDP

#include <stdbool.h>

#define CL_UDP_DISCOVERY_IP_1	224
#define CL_UDP_DISCOVERY_IP_2	0
#define CL_UDP_DISCOVERY_IP_3	0
#define CL_UDP_DISCOVERY_IP_4	142
#define CL_UDP_DISCOVERY_PORT	4551

#define CL_UDP_BUNDLE_PORT		4556


bool convergence_layer_udp_init(void);
int convergence_layer_udp_send_discovery(const uint8_t* const payload, const uint8_t length);

#endif // CONVERGENCE_LAYER_UDP

