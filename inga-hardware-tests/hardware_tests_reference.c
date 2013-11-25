/**
 * @author Robert Hartung, hartung@ibr.cs.tu-bs.de
 */

#include "contiki.h"
#include "net/rime.h"

#include <stdio.h>
#include "test.h"
#include "test-params.h"

static char buff_[30];

TEST_SUITE("net_test_sender");

/*---------------------------------------------------------------------------*/
PROCESS(rime_unicast_sender, "Rime Unicast Sender");
AUTOSTART_PROCESSES(&rime_unicast_sender);
/*---------------------------------------------------------------------------*/
static struct unicast_conn uc;
static uint8_t rec_count = 0;
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{

  printf("unicast message received from %x.%x: '%s'\n", from->u8[0], from->u8[1], (char *) packetbuf_dataptr());
  sprintf(buff_, NET_TEST_CFG_REQUEST_MSG, rec_count);
  printf("packet: %s\n buff: %s\n",(char *) packetbuf_dataptr(), buff_);
  if(strcmp((char *) packetbuf_dataptr(), buff_) == 0){
	  static rimeaddr_t addr;
	  addr.u8[0] = 2 & 0xFF;
	  addr.u8[1] = 0 >> 8;
	  static int8_t idx = 0;
	  char buff_[30] = {'\0'};
	  sprintf(buff_, NET_TEST_CFG_REQUEST_MSG, rec_count);
	  packetbuf_copyfrom(buff_ , NET_TEST_CFG_REQUEST_MSG_LEN); 
	  unicast_send(&uc, &addr);
	  printf("send: %s\n",buff_);
	  rec_count++;
  }
  
  // check if done
  if (rec_count == 10) {
    TEST_PASS();
  }
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rime_unicast_sender, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc));

  PROCESS_BEGIN();


  unicast_open(&uc, 146, &unicast_callbacks); // channel = 145
  while(1){
	PROCESS_YIELD();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
