#include "contiki.h"
#include "net/rime.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
PROCESS(rime_unicast_sender, "Example unicast");
AUTOSTART_PROCESSES(&rime_unicast_sender);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  char *datapntr;
  datapntr = packetbuf_dataptr();
  datapntr[15] = '\0';

  printf("unicast message received from %d.%d: '%s'\n", from->u8[0], from->u8[1], datapntr);
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc}; // List of Callbacks to be called if a message has been received.
static struct unicast_conn uc;
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
