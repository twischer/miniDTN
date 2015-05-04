#include "contiki.h"

PROCESS(dummy_process, "dummy process");
AUTOSTART_PROCESSES(&dummy_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dummy_process, ev, data)
{
  PROCESS_BEGIN();
  while (1) {
    PROCESS_YIELD();
  };
  PROCESS_END();
}


