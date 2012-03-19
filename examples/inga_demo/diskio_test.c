
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

#include "interfaces/flash-microSD.h"           //tested
#include "drv/fat/diskio.h"           //tested
#include <stdio.h> /* For printf() */
#include <avr/wdt.h>
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
	uint16_t i = 0;
	uint8_t buffer[512];
	struct diskio_device_info *info = 0;
	uint32_t start;
	wdt_disable();
	printf("\nTEST BEGIN\n");
	while(diskio_detect_devices() != DISKIO_SUCCESS);
	info = diskio_devices();
	for(i = 0; i < DISKIO_MAX_DEVICES; i++) {
		print_device_info( info + i );
		if( (info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION) ) {
		//if( (info + i)->type == DISKIO_DEVICE_TYPE_GENERIC_FLASH ) {
			info += i;
			break;
		}
	}
	
	start = RTIMER_NOW();
	for(i = 0; i < 30; i++)
		diskio_write_block( info, i, buffer );
	start = RTIMER_NOW() - start;
	printf("\nWrite Time (30 Bytes): %lu", start);
	start = RTIMER_NOW();
	for(i = 0; i < 30; i++)
		diskio_read_block( info, i, buffer );
	start = RTIMER_NOW() - start;
	printf("\nRead Time (30 Bytes): %lu\n", start);
	/*printf("diskio_read_block() = %u", diskio_read_block( info, 0, buffer ) );
	printf("\n");
	for(i = 0; i < 512; i++) {
		printf("%02x", buffer[i]);
		if( ((i+1) % 2) == 0 )
			printf(" ");
		if( ((i+1) % 32) == 0 )
			printf("\n");
	}
	memset( buffer, 0xa, 512 );
	printf("diskio_write_block() = %u", diskio_write_block( info, 0, buffer ) );
	printf("\n");
	memset( buffer, 0x0, 512 );
	printf("diskio_read_block() = %u", diskio_read_block( info, 0, buffer ) );
	printf("\n");
	for(i = 0; i < 512; i++) {
		printf("%02x", buffer[i]);
		if( ((i+1) % 2) == 0 )
			printf(" ");
		if( ((i+1) % 32) == 0 )
			printf("\n");
	}*/
                
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
