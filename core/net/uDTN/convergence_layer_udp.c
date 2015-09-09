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
#include "dispatching.h"
#include "bundle_ageing.h"

static ip_addr_t mcast_addr;
static struct netconn* discovery_conn = NULL;
static struct netconn* bundle_conn = NULL;

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
	if (netbuf_len(buf) != buf->ptr->len) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "Payload is splitted. Possibly not processed correctly.");
	}

	return 0;
}


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
	const err_t err = netconn_join_leave_group(discovery_conn, &mcast_addr, IP_ADDR_ANY, NETCONN_JOIN);
	if (err != ERR_OK) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "netconn_join_leave_group failed with error %d\n", err);
	}


	while (true) {
		static struct netbuf* buf = NULL;
		if (netconn_recv(discovery_conn, &buf) == ERR_OK) {
			const ip_addr_t* const addr = netbuf_fromaddr(buf);
			const uint16_t port = netbuf_fromport(buf);
			LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Discovery package received from addr %s port %u", ipaddr_ntoa(addr), port);

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
 * @brief convergence_layer_udp_incoming_bundle processes an incoming bundle frame
 * @param ip of the sender
 * @param port of the sender
 * @param payload data of the bundle
 * @param lengthsize of the bundle data
 * @return < 0 on fail
 */
static int convergence_layer_udp_incoming_bundle(const ip_addr_t* const ip, const uint16_t port,
	const uint8_t* const payload, const size_t length)
{
	/* Allocate memory, parse the bundle and set reference counter to 1 */
	struct mmem* const bundlemem = bundle_recover_bundle((uint8_t*)payload, length);
	if(bundlemem == NULL) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "Error recovering bundle. Possibly not enough memory.");
		return -1;
	}

	struct bundle_t* const bundle = (struct bundle_t*) MMEM_PTR(bundlemem);
	if(bundle == NULL) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_WRN, "Invalid bundle pointer. Possibly not enough memory.");
		bundle_decrement(bundlemem);
		return -2;
	}

	/* Check for bundle expiration */
	if( bundle_ageing_is_expired(bundlemem) ) {
		LOG(LOGD_DTN, LOG_CL_UDP, LOGL_ERR, "Bundle received from %s:%u is expired", ipaddr_ntoa(ip), port);
		bundle_decrement(bundlemem);
		return -3;
	}

	/* Mark the bundle as "internal" */
	agent_set_bundle_source(bundle);

	LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Bundle from ipn:%lu.%lu (to ipn:%lu.%lu) received from %s:%u",
		bundle->src_node, bundle->src_srv, bundle->dst_node, bundle->dst_srv, ipaddr_ntoa(ip), port);

	/* Store the node from which we received the bundle */
// TODO	linkaddr_copy(&bundle->msrc, source);

	/* Hand over the bundle to dispatching */
	return dispatching_dispatch_bundle(bundlemem);
}


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

			/* Notify the discovery module, that we have seen a peer */
			if (convergence_layer_udp_incoming_bundle(addr, port, data, length) < 0) {
				LOG(LOGD_DTN, LOG_CL_UDP, LOGL_DBG, "Bundle processing from %s:%u failed.", ipaddr_ntoa(addr), port);
			}

			netbuf_delete(buf);
		}
	}
}


/**
 * @brief convergence_layer_udp_init initializes all components for the UDP-CL
 * @return true on success
 */
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
