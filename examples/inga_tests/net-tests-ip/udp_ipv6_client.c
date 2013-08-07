#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>
#include "net/uip-debug.h"
#include "../test.h"
#include "test-params.h"

#define DEBUG DEBUG_PRINT
#define SEND_INTERVAL		CLOCK_SECOND
#define MAX_PAYLOAD_LEN		40

static struct uip_udp_conn *client_conn;
static char* buff_[30] = {'\0'};
static uint8_t done = 0;

TEST_SUITE("udp_ipv6_client");
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  char *str;
  static uint8_t msg_cnt = 0;

  if (uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("Response from the server: '%s'\n", str);

    sprintf(buff_, NET_TEST_CFG_REPLY_MSG, msg_cnt);          

    msg_cnt++;
    if (msg_cnt == 10) {
      TESTS_DONE();
      done = 1;
    } else {
      TEST_EQUALS(strcmp(buff_, str), 0);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
  static int seq_id = 0;
  char buf[MAX_PAYLOAD_LEN];

  printf("Client sending to: ");
  PRINT6ADDR(&client_conn->ripaddr);
  sprintf(buf, NET_TEST_CFG_REQUEST_MSG, seq_id++);
  printf(" (msg: %s)\n", buf);

  uip_udp_packet_send(client_conn, buf, strlen(buf));
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer et;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();

  printf("############################################################\n");

  /* NOTE: Use IPv6 address of server here. */
  uip_ip6addr(&ipaddr, 0xfe80, 0x0000, 0x0000, 0x0000, 
      (NET_TEST_CFG_TARGET_NODE_ID >> 8) | 0x0200 | ((NET_TEST_CFG_TARGET_NODE_ID & 0xFF) << 8),
      0x22ff, 0xfe33, 0x4455);

  /* new connection with remote host */
  client_conn = udp_new(&ipaddr, UIP_HTONS(3000), NULL);
  udp_bind(client_conn, UIP_HTONS(3034));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
          UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  etimer_set(&et, SEND_INTERVAL);

  static int8_t idx;
  while (!done) {

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
