
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
		uint8_t obuffer[514];
		uint8_t ibuffer[514];
		uint16_t j, i;
  		uint32_t start, end;
		//printf("\nChecksum is: %x", microSD_data_crc( buffer ));
		//wdt_disable();
		init:
		printf("\nmicroSD_init() = %u\n", i = microSD_init());
		if( i != 0 )
			goto init;
		printf("Size of uint64_t  = %u\n", sizeof(uint64_t));
		printf("Size of uint32_t  = %u\n", sizeof(uint32_t));
		printf("Size of uint16_t  = %u\n", sizeof(uint16_t));
		printf("Size of int  = %u\n", sizeof(int));
/*		for (j = 0; j < 512; j+=4) {
				obuffer[j] = 'H';
				obuffer[j + 1] = 'e';
				obuffer[j + 2] = 'r';
				obuffer[j + 3] = '\n';

		}*/
		//clock_init();
		//printf("\nmicroSD_set_CRC() = %u", microSD_set_CRC(1));
		//start = clock_time();
		/*for( j = 500; j < 8192; j++ ) {
			printf("\n%u", j);
			retry_write:
			if( microSD_write_block(0L + j, obuffer) != 0 ) {
				printf("\n Error writing block %lu", 0L + j);
				//goto retry_write;
			}
			watchdog_periodic();
			retry_read:
			if( microSD_read_block(0L+j, ibuffer) != 0) {
				goto retry_read;
			}
			watchdog_periodic();
			for (i = 0; i < 512; i+=4) {
				watchdog_periodic();
					if( ibuffer[i] != 'H' || ibuffer[i + 1] != 'e' || ibuffer[i + 2] != 'r' || ibuffer[i + 3] != '\n' ) {
						printf("\n Error in block %u", j);
						for(i = 0; i< 512; i++) {
							if( i%2 == 0 ) {
								printf(" ");
							}
							if( i%32 == 0){
								printf("\n");
							}
							printf("%02x", ibuffer[i]);
						}
						break;
					}
			}
			printf("\nChecksum is       : %x", ( (uint16_t)ibuffer[512] << 8 ) + ibuffer[513]);
			printf("\nChecksum should be: %x", microSD_data_crc( ibuffer ));
		}*/
		rtimer_arch_init();
		start = RTIMER_NOW();
		for( j = 0; j < 30; j++ ) {
			microSD_write_block( j, obuffer );
		}
		end = RTIMER_NOW();
		printf("\nWrite time = %lu (%lu)", end - start, (end - start) / 30);
		rtimer_arch_init();
		start = RTIMER_NOW();
		for( j = 0; j < 30; j++ ) {
			if( microSD_read_block( j, obuffer ) != 0 ) {
				printf("\n Block %u read error", j);
				microSD_init();
				j--;
				continue;
			}
		}
		end = RTIMER_NOW();
		printf("\nRead time = %lu (%lu)", end - start, (end - start) / 30);
		printf("\nSecond = %lu", RTIMER_SECOND);
		printf("\n");
		//if( microSD_write_block(38000L + j, buffer) != 0 ) {
		//	printf("\n Error writing block %lu", 38000L + j);
		//}

		//end = clock_time();
		//printf("\nTime = %lu", (end - start) );
		//printf("\nSecond = %lu", CLOCK_SECOND );
/*
		for (i = 0; i < 8192; i++) {
			if( (j = microSD_read_block(0L+i, buffer)) != 0 ) {
				printf("\nmicroSD_read_block() failed with %u", j);
			}
			for (j = 0; j < 512; j+=4) {
					if( buffer[j] != 'f' ||	buffer[j + 1] != 'e' ||	buffer[j + 2] != 'r' || buffer[j + 3] != '\n' ) {
						printf("\n Error in block %u", i);
						break;
					}
			}
		}
*/
		microSD_switchoff();
                
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
