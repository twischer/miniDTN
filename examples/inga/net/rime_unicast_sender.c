/**
 * @author Robert Hartung, hartung@ibr.cs.tu-bs.de
 * @author Dennis Reimers, d.reimers@tu-bs.de
 */

#include "contiki.h"
#include "net/rime/rime.h"

#include <stdio.h>
#include <avr/pgmspace.h>

// TODO: Change ADDR to your receiver node, eg. lookup link addr on receiver boot
#define RECEIVER_LINK_ADDR 0x04d6

#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
/*---------------------------------------------------------------------------*/
PROCESS(rime_unicast_sender, "Rime Unicast Sender");
AUTOSTART_PROCESSES(&rime_unicast_sender);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  PRINTF("unicast message received from %x.%x: '%s'\n", from->u8[0], from->u8[1], (char *) packetbuf_dataptr());
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rime_unicast_sender, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc));

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks); // channel = 146

  while (1) {
    static struct etimer et;
    linkaddr_t addr;

    etimer_set(&et, CLOCK_SECOND);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("Unicast Example", 15); // String + Length to be send

    // Address of receiving Node
    addr.u8[0] = RECEIVER_LINK_ADDR & 0xFF;
    addr.u8[1] = RECEIVER_LINK_ADDR >> 8;

    if (!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
      PRINTF("Message sent\n"); // debug message
      unicast_send(&uc, &addr);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
