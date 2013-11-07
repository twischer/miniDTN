#include "contiki.h"

PROCESS(dummy_process, "dummy process");
AUTOSTART_PROCESSES(&dummy_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(dummy_process, ev, data)
{
}


