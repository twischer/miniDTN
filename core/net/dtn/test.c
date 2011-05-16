#include "contiki.h"

#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/dtn/API_registration.h"
#include "net/dtn/API_events.h"
#include "net/dtn/agent.h"
#include "dev/leds.h"
#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/

PROCESS(test_process, "test process");
AUTOSTART_PROCESSES(&test_process);
  static struct registration_api reg;
  static struct etimer timer;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();
  	printf("started test process\n");
	while(1) {
		etimer_set(&timer, CLOCK_SECOND);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
			uint32_t foo= clock_time();
			printf("test timer %lu\n",foo);
		}
  PROCESS_END();
}

void test_init(void){
	process_start(&test_process, NULL);
}
