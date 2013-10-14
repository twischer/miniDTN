/*
 * set_nodeid.c
 *
 *  Created on: 13.10.2011
 *      Author: Georg von Zengen 

 *	to set node_id, give NODE_ID=<x> with make
 *
 */

#include "contiki.h"
#include <avr/eeprom.h>
#include "settings.h"
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include "node-id.h"


#define DEBUG                                   0

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(default_app_process, "Hello world process");
AUTOSTART_PROCESSES(&default_app_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;

PROCESS_THREAD(default_app_process, ev, data) {
  PROCESS_BEGIN();
  printf("setting new node_id\n");


  etimer_set(&timer, CLOCK_SECOND * 0.05);
#ifdef NODEID
  uint16_t node_id2 = NODEID;
  settings_set_uint16(SETTINGS_KEY_PAN_ADDR, node_id2);
#endif
  while (1) {

    //		settings_wipe();
    PROCESS_YIELD();
    etimer_set(&timer, CLOCK_SECOND);
//    printf("node_id = %u\n", node_id);

  }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
