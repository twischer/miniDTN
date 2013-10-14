/**
 * @author Robert Hartung, hartung@ibr.cs.tu-bs.de
 */

#include "contiki.h"
#include "net/rime.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/

PROCESS(rime_unicast_sender, "Rime Unicast Sender");
AUTOSTART_PROCESSES(&rime_unicast_sender);
/*---------------------------------------------------------------------------*/

static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
	char *datapntr;
	datapntr = packetbuf_dataptr();
	datapntr[15] = '\0';
  printf("unicast message received from %02d.%02d: '%s'\n", from->u8[0], from->u8[1], datapntr);
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(rime_unicast_sender, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc));

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks); // channel = 145

  while (1) {
    static struct etimer et;
    rimeaddr_t addr;

    etimer_set(&et, CLOCK_SECOND);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("Unicast Example", 15); // String + Length to be send

    addr.u8[0] = 0x21;
    addr.u8[1] = 0; // Address of receiving Node

    if (!rimeaddr_cmp(&addr, &rimeaddr_node_addr)) {
      printf("Message sent\n"); // debug message
      unicast_send(&uc, &addr);
    }
  }

  PROCESS_END();
}
