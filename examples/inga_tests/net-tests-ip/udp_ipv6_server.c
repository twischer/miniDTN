#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>
#include "net/uip-debug.h"
#include "../test.h"
#include "test-params.h"

#define UIP_IP_BUF    ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF   ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])

#define MAX_PAYLOAD_LEN 120
#define PRINTF  printf

static struct uip_udp_conn *server_conn;

TEST_SUITE("udp_ipv6_server");

PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  static int seq_id = 0;
  char buf[MAX_PAYLOAD_LEN];

  if (uip_newdata()) {
    ((char *) uip_appdata)[uip_datalen()] = '\0';
    PRINTF("Server received: '%s' from ", (char *) uip_appdata);
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF("\n");

    sprintf(buf, NET_TEST_CFG_REQUEST_MSG, seq_id);
    printf("I received: %s\n", (char*) uip_appdata);
    printf("Should be : %s\n", (char*) buf);
    TEST_EQUALS(strcmp(uip_appdata, buf), 0);

    // reply message
    sprintf(buf, NET_TEST_CFG_REPLY_MSG, seq_id++);
    PRINTF("Responding with message: %s\n", buf);

    // return to sender
    uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
    uip_udp_conn->rport = UIP_UDP_BUF->destport = UIP_UDP_BUF->srcport;
    uip_udp_packet_send(server_conn, buf, strlen(buf));
    /* Restore server connection to allow data from any node */
    memset(&server_conn->ripaddr, 0, sizeof (server_conn->ripaddr));

    if (seq_id == 10) {
      TESTS_DONE();
      exit(0);
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  printf("############################################################\n");

  server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));

  while (1) {
    PROCESS_YIELD();
    if (ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
