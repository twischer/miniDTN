/**
 * \file
 * \brief UDP Convergence Layer Implementation
 * \author Timo Wischer <wischer@ibr.cs.tu-bs.de>
 */

#include "convergence_layer_udp.h"

#include "FreeRTOS.h"
#include "task.h"
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lib/logging.h"

#include "agent.h"
#include "discovery.h"
#include "dispatching.h"
#include "bundle_ageing.h"
#include "convergence_layer.h"
#include "convergence_layer_udp_dgram.h"

ip_addr_t udp_mcast_addr;
static struct netconn* bundle_conn = NULL;

#ifdef UDP_DISCOVERY_ANNOUNCEMENT
static struct netconn* discovery_conn = NULL;
#endif /* UDP_DISCOVERY_ANNOUNCEMENT */



/**
 * @brief convergence_layer_udp_netbuf_to_array convertes a netbuf to an array
 * @param buf the netbuf containig the package data
 * @param data a pointer to the memory where
 * the address to the payload array should be safed
 * @param length a pointer to the memory where the
 * length of the data array should be safed
 * @return < 0 on fail
 */
static int convergence_layer_udp_netbuf_to_array(struct netbuf* buf, const uint8_t** data, uint16_t* const length)
{
	if (netbuf_data(buf, (void**)data, length) != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "Payload is empty");
		return -1;
	}

	// TODO merge the data, if the package was splitted
	// see netbuf_copy
	if (netbuf_len(buf) != buf->ptr->len) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "Payload is splitted. Possibly not processed correctly.");
	}

	return 0;
}


static int convergence_layer_udp_send(struct netconn* const conn, const ip_addr_t* const addr, const uint16_t port,
									  const uint8_t* const payload, const size_t length,
									  const uint8_t* const payload2, const size_t length2)
{
	/*
	 * conn has not to be checked for NULL,
	 * because it will be done by netconn_sendto()
	 */

	if (!netif_is_up(netif_default)) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "Network interface is down. Could not send udp data.");
		return -5;
	}

	struct netbuf* const buf = netbuf_new();
	if (buf == NULL) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Not enough free memory for allocating a new netbuf.");
		return -1;
	}

	if (netbuf_ref(buf, payload, length) != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Not enough free memory for allocating a new pbuf.");
		netbuf_delete(buf);
		return -2;
	}

	/* add second buffer to list, if existing */
	if (payload2 != NULL && length2 > 0) {
		struct pbuf* const pbuf = pbuf_alloc(PBUF_RAW, 0, PBUF_REF);
		if (pbuf == NULL) {
			LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Not enough free memory for allocating a second new pbuf.");
			netbuf_delete(buf);
			return -3;
		}

		pbuf->payload = (uint8_t*)payload2;
		pbuf->len = pbuf->tot_len = length2;
		pbuf_cat(buf->p, pbuf);
	}


	if (netconn_sendto(conn, buf, (ip_addr_t*)addr, port) != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Could not send data. Buffer is not existing.");
		netbuf_delete(buf);
		return -4;
	}

	netbuf_delete(buf);
	return 0;
}


#ifdef UDP_DISCOVERY_ANNOUNCEMENT
/**
 * @brief convergence_layer_udp_discovery_thread processes the incoming discovery messages
 * @param arg is not used
 */
static void convergence_layer_udp_discovery_thread(void *arg)
{
	LWIP_UNUSED_ARG(arg);

	/* block until interface is up */
	while (!netif_is_up(netif_default)) {
		vTaskDelay(100);
	}

	/* join to the multicast group for the discovery messages */
	const err_t err = netconn_join_leave_group(discovery_conn, &udp_mcast_addr, IP_ADDR_ANY, NETCONN_JOIN);
	if (err != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "netconn_join_leave_group failed with error %d\n", err);
	}


	while (true) {
		static struct netbuf* buf = NULL;
		if (netconn_recv(discovery_conn, &buf) == ERR_OK) {
			const ip_addr_t* const addr = netbuf_fromaddr(buf);

#ifdef ENABLE_LOGGING
			const uint16_t port = netbuf_fromport(buf);
			// TODO replace ipaddr_ntoa with ipaddr_ntoa_r to make it reentrant for multi threading
			LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Discovery package received from addr %s port %u", ipaddr_ntoa(addr), port);
#endif /* ENABLE_LOGGING */

			const uint8_t* data = 0;
			uint16_t length = 0;
			if (convergence_layer_udp_netbuf_to_array(buf, &data, &length) < 0) {
				netbuf_delete(buf);
				continue;
			}

			/* Notify the discovery module, that we have seen a peer */
			DISCOVERY.receive_ip(addr, data, length);

			netbuf_delete(buf);
		}
	}
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
	return convergence_layer_udp_send(discovery_conn, &udp_mcast_addr, CL_UDP_DISCOVERY_PORT, payload, length, NULL, 0);
}
#endif /* UDP_DISCOVERY_ANNOUNCEMENT */


/**
 * @brief convergence_layer_udp_bundle_thread processes the incoming bundles
 * @param arg is not used
 */
static void convergence_layer_udp_bundle_thread(void *arg)
{
	LWIP_UNUSED_ARG(arg);

	while (true) {
		static struct netbuf* buf = NULL;
		if (netconn_recv(bundle_conn, &buf) == ERR_OK) {
			const ip_addr_t* const addr = netbuf_fromaddr(buf);
			const uint16_t port = netbuf_fromport(buf);
			LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Bundle package received from addr %s port %u", ipaddr_ntoa(addr), port);

			const uint8_t* data = 0;
			uint16_t length = 0;
			if (convergence_layer_udp_netbuf_to_array(buf, &data, &length) < 0) {
				netbuf_delete(buf);
				continue;
			}

			cl_addr_t source;
			source.isIP = true;
			ip_addr_copy(source.ip, *addr);
			source.port = port;
			convergence_layer_udp_dgram_incoming_frame(&source, data, length, 0);

			netbuf_delete(buf);
		}
	}
}


int convergence_layer_udp_send_data(const ip_addr_t* const addr, const uint8_t* const payload, const uint8_t length,
									const uint8_t* const payload2, const uint8_t length2)
{
	return convergence_layer_udp_send(bundle_conn, addr, CL_UDP_BUNDLE_PORT, payload, length, payload2, length2);
}


/**
 * @brief convergence_layer_udp_init initializes all components for the UDP-CL
 * @return true on success
 */
bool convergence_layer_udp_init(void)
{
	IP4_ADDR(&udp_mcast_addr, CL_UDP_DISCOVERY_IP_1, CL_UDP_DISCOVERY_IP_2, CL_UDP_DISCOVERY_IP_3, CL_UDP_DISCOVERY_IP_4);


#ifdef UDP_DISCOVERY_ANNOUNCEMENT
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
#endif /* UDP_DISCOVERY_ANNOUNCEMENT */


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
