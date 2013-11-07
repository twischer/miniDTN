/**
 * @author Robert Hartung, hartung@ibr.cs.tu-bs.de
 */

#include "contiki.h"
#include "net/rime.h"

#include <stdio.h>
#include "../test.h"
#include "test-params.h"

static char buff_[30];

TEST_SUITE("net_test_sender");

/*---------------------------------------------------------------------------*/
PROCESS(rime_unicast_sender, "Rime Unicast Sender");
AUTOSTART_PROCESSES(&rime_unicast_sender);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  static uint8_t rec_count = 0;

  printf("unicast message received from %x.%x: '%s'\n", from->u8[0], from->u8[1], (char *) packetbuf_dataptr());
  sprintf(buff_, NET_TEST_CFG_REPLY_MSG, rec_count);
  TEST_EQUALS(strcmp((char *) packetbuf_dataptr(), buff_), 0);

  rec_count++;

  // check if done
  if (rec_count == 10) {
    TESTS_DONE();
  }
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rime_unicast_sender, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc));

  PROCESS_BEGIN();


  unicast_open(&uc, 146, &unicast_callbacks); // channel = 145

  static struct etimer et;
  static rimeaddr_t addr;

  etimer_set(&et, 2*CLOCK_SECOND);

  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  // set address
  addr.u8[0] = NET_TEST_CFG_TARGET_NODE_ID & 0xFF;
  addr.u8[1] = NET_TEST_CFG_TARGET_NODE_ID >> 8;

  // send 10 messages
  static int8_t idx = 0;
  char buff_[30] = {'\0'};
  for (idx = 0; idx < 10; idx++) {
    etimer_set(&et, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    sprintf(buff_, NET_TEST_CFG_REQUEST_MSG, idx);
    packetbuf_copyfrom(buff_ , NET_TEST_CFG_REQUEST_MSG_LEN); 
    unicast_send(&uc, &addr);
    printf("sent: %s\n", buff_);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
