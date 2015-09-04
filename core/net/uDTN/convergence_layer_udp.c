/**
 * \file
 * \brief UDP Convergence Layer Implementation
 * \author Timo Wischer <wischer@ibr.cs.tu-bs.de>
 */

#include "FreeRTOS.h"
#include "task.h"
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lib/logging.h"

#include "convergence_layer_udp.h"
#include "agent.h"


static struct netconn* discovery_conn = NULL;
static struct netconn* bundle_conn = NULL;


static void convergence_layer_udp_discovery_thread(void *arg)
{
	LWIP_UNUSED_ARG(arg);

	/* block until interface is up */
	while (!netif_is_up(netif_default)) {
		vTaskDelay(100);
	}

	/* join to the multicast group for the discovery messages */
	{
		struct ip_addr mcast_addr;
		IP4_ADDR(&mcast_addr, CL_UDP_DISCOVERY_IP_1, CL_UDP_DISCOVERY_IP_2, CL_UDP_DISCOVERY_IP_3, CL_UDP_DISCOVERY_IP_4);

		const err_t err = netconn_join_leave_group(discovery_conn, &mcast_addr, IP_ADDR_ANY, NETCONN_JOIN);
		if (err != ERR_OK) {
			printf("ERR: netconn_join_leave_group failed with error %d\n", err);
		}
	}


	while (1) {
		static struct netbuf* buf = NULL;
		if (netconn_recv(discovery_conn, &buf) == ERR_OK) {
			struct ip_addr* addr = netbuf_fromaddr(buf);
			unsigned short port = netbuf_fromport(buf);
			LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Discovery package received from addr 0x%08x port %u", addr->addr, port);

			netconn_connect(discovery_conn, addr, port);
			buf->addr.addr = 0;
			netconn_send(discovery_conn,buf);
			netbuf_delete(buf);
		}
	}
}


static void convergence_layer_udp_bundle_thread(void *arg)
{
	LWIP_UNUSED_ARG(arg);

	while (1) {
		static struct netbuf* buf = NULL;
		if (netconn_recv(bundle_conn, &buf) == ERR_OK) {
			struct ip_addr* addr = netbuf_fromaddr(buf);
			unsigned short port = netbuf_fromport(buf);
			netconn_connect(bundle_conn, addr, port);
			buf->addr.addr = 0;
			netconn_send(bundle_conn,buf);
			netbuf_delete(buf);
		}
	}
}


bool convergence_layer_udp_init(void)
{
	logging_domain_level_set(LOGD_DTN, LOG_CL_UDP, LOGL_DBG);

	/* initalize the udp connection for the discovery messages */
	discovery_conn = netconn_new(NETCONN_UDP);
	if (discovery_conn == NULL) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "netconn_new failed\n");
		return false;
	}

	if (netconn_bind(discovery_conn, IP_ADDR_ANY, CL_UDP_DISCOVERY_PORT) != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "ERR: netconn_bind failed\n");
		netconn_delete(discovery_conn);
		return false;
	}

	if ( !xTaskCreate(convergence_layer_udp_discovery_thread, "UDP-CL discovery", configMINIMAL_STACK_SIZE+100, NULL, 1, NULL) ) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "UDP-CL discovery task creation failed.");
		return false;
	}

	/* initalize the udp connection for the bundle data */
	bundle_conn = netconn_new(NETCONN_UDP);
	if (bundle_conn == NULL) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "netconn_new failed\n");
		return false;
	}

	if (netconn_bind(bundle_conn, IP_ADDR_ANY, CL_UDP_BUNDLE_PORT) != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "netconn_bind failed\n");
		netconn_delete(bundle_conn);
		return false;
	}

	if ( !xTaskCreate(convergence_layer_udp_bundle_thread, "UDP-CL bundle", configMINIMAL_STACK_SIZE+100, NULL, 1, NULL) ) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "UDP-CL bundle task creation failed.");
		return false;
	}


	LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "UDP-CL tasks init done.");
	return true;
}
