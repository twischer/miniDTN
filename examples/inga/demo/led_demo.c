
/*
 * set_nodeid.c
 *
 *  Created on: 13.10.2011
 *      Author: Georg von Zengen 

 *	to set node_id, give NODE_ID=<x> with make
 *
 */
	
#include "contiki.h"
#ifdef INGA_REVISION
#include <avr/eeprom.h>
#include "settings.h"
#include <util/delay.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "node-id.h"
#include "leds.h"


#define DEBUG                                   0

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(default_app_process, "Hello world process");
AUTOSTART_PROCESSES(&default_app_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(default_app_process, ev, data)
{
  PROCESS_BEGIN();

	leds_init();	
	leds_on(LEDS_GREEN);
	leds_off(LEDS_YELLOW);
	etimer_set(&timer,  CLOCK_SECOND*0.05);
        while (1) {
		
		PROCESS_YIELD();
		etimer_set(&timer,  CLOCK_SECOND);
		leds_toggle(LEDS_GREEN | LEDS_YELLOW); // toggle both LEDs at once using bitwise or

        }

  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
