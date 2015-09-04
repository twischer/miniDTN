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


static struct netconn *conn;
static struct netbuf *buf;
static struct ip_addr *addr;
static unsigned short port;
/*-----------------------------------------------------------------------------------*/
static void convergence_layer_udp_thread(void *arg)
{
  err_t err, recv_err;
  
  LWIP_UNUSED_ARG(arg);

  conn = netconn_new(NETCONN_UDP);
  if (conn == NULL)
  {
	  printf("netconn_new failed\n");
	  vTaskDelete(NULL);
  }
  else
  {
	err = netconn_bind(conn, IP_ADDR_ANY, 1234);
	if (err == ERR_OK)
	{
	  while (1)
	  {
		recv_err = netconn_recv(conn, &buf);
      
		if (recv_err == ERR_OK)
		{
		  addr = netbuf_fromaddr(buf);
		  port = netbuf_fromport(buf);
		  netconn_connect(conn, addr, port);
		  buf->addr.addr = 0;
		  netconn_send(conn,buf);
		  netbuf_delete(buf);
		}
	  }
	}
	else
	{
	  netconn_delete(conn);
	}
  }
}


bool convergence_layer_udp_init(void)
{
  if ( !xTaskCreate(convergence_layer_udp_thread, "UDP-CL process", configMINIMAL_STACK_SIZE+100, NULL, 1, NULL) ) {
	  return false;
  }

  return true;
}
