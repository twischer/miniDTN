
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
#include <avr/wdt.h>


#include "interfaces/flash-microSD.h"           //tested
#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
        //microSD Test
		uint8_t buffer[512];
		uint16_t j, i;
  		uint32_t start, end;
		memset(buffer, 0xFF, 512);
		printf("\nChecksum is: %x", microSD_data_crc( buffer ));
		//wdt_disable();
		init:
		printf("\nmicroSD_init() = %u\n", i = microSD_init());
		if( i != 0 )
			goto init;
		printf("Size of uint64_t  = %u\n", sizeof(uint64_t));
		printf("Size of uint32_t  = %u\n", sizeof(uint32_t));
		printf("Size of uint16_t  = %u\n", sizeof(uint16_t));
		printf("Size of int  = %u\n", sizeof(int));
		for (j = 0; j < 512; j+=4) {
				buffer[j] = 'u';
				buffer[j + 1] = 'e';
				buffer[j + 2] = 'r';
				buffer[j + 3] = '\n';

		}
		clock_init();
		start = clock_time();
		for( j = 0; j < 8192; j++ ) {
			//printf("\n%u", j);
			if( microSD_write_block(38000L + j, buffer) != 0 ) {
				printf("\n Error writing block %lu", 38000L + j);
			}
		}
		end = clock_time();
		printf("\nTime = %lu", (end - start) );
		printf("\nSecond = %lu", CLOCK_SECOND );
/*
		for (i = 0; i < 512; i++) {
			microSD_read_block(12+i, buffer);
			for (j = 0; j < 512; j+=4) {
					if( buffer[j] != 'u' ||	buffer[j + 1] != 'e' ||	buffer[j + 2] != 'r' || buffer[j + 3] != '\n' ) {
						printf("\n Error in block %u", i);
						break;
					}
			}
		}
*/
		microSD_deinit();
                
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
