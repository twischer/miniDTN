#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

#include "net/uip-debug.h"

#define DEBUG DEBUG_PRINT
#define SEND_INTERVAL		15 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN		40

static struct uip_udp_conn *client_conn;
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  char *str;

  if (uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("Response from the server: '%s'\n", str);
  }
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
  static int seq_id;
  char buf[MAX_PAYLOAD_LEN];

  printf("Client sending to: ");
  PRINT6ADDR(&client_conn->ripaddr);
  sprintf(buf, "Hello %d from the client", ++seq_id);
  printf(" (msg: %s)\n", buf);

  uip_udp_packet_send(client_conn, buf, strlen(buf));
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer et;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();

  /* NOTE: Use IPv6 address of server here. */
  uip_ip6addr(&ipaddr, 0xfe80, 0x0000, 0x0000, 0x0000, 0x2f00, 0x22ff, 0xfe33, 0x4455);

  /* new connection with remote host */
  client_conn = udp_new(&ipaddr, UIP_HTONS(3000), NULL);
  udp_bind(client_conn, UIP_HTONS(3034));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
          UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  etimer_set(&et, SEND_INTERVAL);
  while (1) {
    PROCESS_YIELD();
    if (etimer_expired(&et)) {
      timeout_handler();
      etimer_restart(&et);
    } else if (ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
