#ifndef CONVERGENCE_LAYER_UDP
#define CONVERGENCE_LAYER_UDP

#include <stdbool.h>
#include <lwip/ip_addr.h>


#define UDP_DISCOVERY_ANNOUNCEMENT		1

#define CL_UDP_DISCOVERY_IP_1	224
#define CL_UDP_DISCOVERY_IP_2	0
#define CL_UDP_DISCOVERY_IP_3	0
#define CL_UDP_DISCOVERY_IP_4	142
#define CL_UDP_BUNDLE_PORT		4565

#ifdef UDP_DISCOVERY_ANNOUNCEMENT
	#define CL_UDP_DISCOVERY_PORT	4551
#endif /* UDP_DISCOVERY_ANNOUNCEMENT */


ip_addr_t udp_mcast_addr;

int convergence_layer_udp_init(void);
int convergence_layer_udp_send_data(const ip_addr_t* const addr, const uint8_t* const payload, const size_t length,
									const uint8_t* const payload2, const size_t length2);

#ifdef UDP_DISCOVERY_ANNOUNCEMENT
int convergence_layer_udp_send_discovery(const uint8_t* const payload, const size_t length);
#endif /* UDP_DISCOVERY_ANNOUNCEMENT */

#endif // CONVERGENCE_LAYER_UDP

