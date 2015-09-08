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
#include "discovery.h"

static ip_addr_t mcast_addr;
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
	const err_t err = netconn_join_leave_group(discovery_conn, &mcast_addr, IP_ADDR_ANY, NETCONN_JOIN);
	if (err != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "netconn_join_leave_group failed with error %d\n", err);
	}


	while (1) {
		static struct netbuf* buf = NULL;
		if (netconn_recv(discovery_conn, &buf) == ERR_OK) {
			struct ip_addr* addr = netbuf_fromaddr(buf);
			unsigned short port = netbuf_fromport(buf);
			LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Discovery package received from addr %s port %u", ipaddr_ntoa(addr), port);

			// TODO merge the data, if the package was splitted
			const uint8_t* data = 0;
			uint16_t length = 0;
			if (netbuf_data(buf, (void**)&data, &length) != ERR_OK) {
				LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "Discovery package payload from addr %s port %u is empty", ipaddr_ntoa(addr), port);
				netbuf_delete(buf);
				continue;
			}

			if (netbuf_len(buf) != buf->ptr->len) {
				LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "Discovery package payload from addr %s port %u splitted. Possibly not processed correctly.",
					ipaddr_ntoa(addr), port);
			}

			/* Notify the discovery module, that we have seen a peer */
			DISCOVERY.receive_ip(addr, data, length);

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
	// TODO only for debugging
	// have to be removed if the udp-cl implementtation has finisched
	logging_domain_level_set(LOGD_DTN, LOG_CL_UDP, LOGL_DBG);
	logging_domain_level_set(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG);


	IP4_ADDR(&mcast_addr, CL_UDP_DISCOVERY_IP_1, CL_UDP_DISCOVERY_IP_2, CL_UDP_DISCOVERY_IP_3, CL_UDP_DISCOVERY_IP_4);

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


/**
 * @brief convergence_layer_udp_send_discovery sends a discovery message as broadcast
 * on the ethernet
 * @param payload
 * @param length
 * @return
 */
int convergence_layer_udp_send_discovery(const uint8_t* const payload, const uint8_t length)
{
	struct netbuf* const buf = netbuf_new ();
	if (buf == NULL) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Not enough free memory for allocating a new netbuf.");
		return -1;
	}

	if (netbuf_ref(buf, payload, length) != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Not enough free memory for allocating a new pbuf.");
		netbuf_delete(buf);
		return -2;
	}

	if (netconn_sendto(discovery_conn, buf, &mcast_addr, CL_UDP_DISCOVERY_PORT) != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Could not send discovery message. Buffer is not existing.");
		netbuf_delete(buf);
		return -2;
	}

	netbuf_delete(buf);
	return 0;
}
