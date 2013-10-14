#include "contiki.h"
#include "net/rime.h"

#include <stdio.h>
#include "../test.h"
#include "test-params.h"

static struct unicast_conn uc;
static char buff_[30];

TEST_SUITE("net_test_receiver");
/*---------------------------------------------------------------------------*/
PROCESS(rime_unicast_sender, "Example unicast");
AUTOSTART_PROCESSES(&rime_unicast_sender);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  char *datapntr;
  static uint8_t rec_count = 0;
  datapntr = packetbuf_dataptr();
  datapntr[NET_TEST_CFG_REQUEST_MSG_LEN] = '\0';

  printf("unicast message received from %x.%x: '%s'\n", from->u8[0], from->u8[1], datapntr);

  sprintf(buff_, NET_TEST_CFG_REQUEST_MSG, rec_count);
  TEST_EQUALS(strcmp((char *) datapntr, buff_), 0);

  sprintf(buff_, NET_TEST_CFG_REPLY_MSG, rec_count);
  packetbuf_copyfrom(buff_, NET_TEST_CFG_REPLY_MSG_LEN); 
  unicast_send(&uc, from);

  rec_count++;

  if (rec_count == 10) {
    TESTS_DONE();
  }
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc}; // List of Callbacks to be called if a message has been received.
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(rime_unicast_sender, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc));

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks); // Channel

  while (1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
