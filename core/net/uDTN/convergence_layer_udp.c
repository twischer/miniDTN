/**
 * \file
 * \brief UDP Convergence Layer Implementation
 * \author Timo Wischer <wischer@ibr.cs.tu-bs.de>
 */

#include "FreeRTOS.h"
#include "task.h"
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/sys.h"

#include "convergence_layer_udp.h"

static struct netconn* discovery_conn = 0;

static void convergence_layer_udp_discovery_thread(void *arg)
{
	LWIP_UNUSED_ARG(arg);

	discovery_conn = netconn_new(NETCONN_UDP);
	if (discovery_conn == NULL) {
		printf("netconn_new failed\n");
		vTaskDelete(NULL);
		return;
	}

	if (netconn_bind(discovery_conn, IP_ADDR_ANY, CL_UDP_DISCOVERY_PORT) != ERR_OK) {
		printf("netconn_bind failed\n");
		netconn_delete(discovery_conn);
		vTaskDelete(NULL);
		return;
	}

	while (1) {
		static struct netbuf* buf = 0;
		if (netconn_recv(discovery_conn, &buf) == ERR_OK) {
			struct ip_addr* addr = netbuf_fromaddr(buf);
			unsigned short port = netbuf_fromport(buf);
			netconn_connect(discovery_conn, addr, port);
			buf->addr.addr = 0;
			netconn_send(discovery_conn,buf);
			netbuf_delete(buf);
		}
	}
}



bool convergence_layer_udp_init(void)
{
	if ( !xTaskCreate(convergence_layer_udp_discovery_thread, "UDP-CL discovery", configMINIMAL_STACK_SIZE+100, NULL, 1, NULL) ) {
		return false;
	}

	return true;
}
