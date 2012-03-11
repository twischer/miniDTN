
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


#include "interfaces/flash-microSD.h"           //tested
#include "drv/fat/diskio.h"           //tested
#include "drv/fat/fat_coop.h"           //tested
#include <stdio.h> /* For printf() */
#include <avr/wdt.h>
#include "dev/watchdog.h"
#include "clock.h"
PROCESS(second_process, "Second Process");
PROCESS_THREAD(second_process, ev, data)
{
	PROCESS_BEGIN();
	static uint8_t buffer[10];
  	static uint32_t start1 = 0, start2 = 0, start3 = 0, start4 = 0, end = 0, begin = 0, count = 0, proc_start = 0;
	static uint8_t token1 = 0, token2 = 0, token3 = 0, token4 = 0;
	static int fd, ret;
	static int i;
	static Event_OperationFinished *event;
	printf("\nSecond process startet");
	proc_start = RTIMER_NOW();
	fd = ccfs_open("LATEX.pdf", CFS_WRITE, &token1);
	do {
		PROCESS_WAIT_EVENT();
	} while( ev != get_coop_event_id() );
	event = (Event_OperationFinished *) data;
	fd = event->ret_value;
	printf("\nfd = %d", fd );

	for( i = 0; i < 5; i++) {
		start1 = RTIMER_NOW();
		ret = ccfs_write( fd, buffer, 10, &token1 );
			//printf("\ntoken1 = %d, %d", token1, ret);
		start2 = RTIMER_NOW();
		ccfs_write( fd, buffer, 10, &token2 );
			//printf("\ntoken2 = %d", token2);
		start3 = RTIMER_NOW();
		ccfs_write( fd, buffer, 10, &token3 );
			//printf("\ntoken3 = %d", token3);
		start4 = RTIMER_NOW();
		ccfs_write( fd, buffer, 10, &token4 );
			//printf("\ntoken4 = %d", token4);
		begin = 0;
		do {
			PROCESS_WAIT_EVENT();
			end = RTIMER_NOW();
			//printf("\nSecond Process got event %d", ev);
			if( ev == get_coop_event_id() ) {
				event = (Event_OperationFinished *) data;
				//printf("\nevent->token = %d", event->token);
				if( event->token == token1 ) {
					count += (end - start1);
					//printf("\nFirst function");
				} else if( event->token == token2 ) {
					count += (end - start2);
					//printf("\nSecond function");
				} else if( event->token == token3 ) {
					count += (end - start3);
					//printf("\nThird function");
				} else if( event->token == token4) {
					count += (end - start4);
					//printf("\nFourth function");
				}
				begin++;
				//printf("\nbegin = %d", begin);
			}
			if( begin == 4 )
				break;
			else
				continue;
		} while( 1 );
	}
	proc_start = RTIMER_NOW() - proc_start;
	printf("\nwrite count: %lu", count);
	printf("\nproc time: %lu\n", proc_start);
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(hello_world_process, ev, data)
{
	PROCESS_BEGIN();
  	static uint32_t start1 = 0, start2 = 0, start3 = 0, start4 = 0, end = 0, begin = 0, count = 0, prog_start = 0;;
	static uint8_t token1 = 0, token2 = 0, token3 = 0, token4 = 0;
	static uint16_t i = 0;
	static uint8_t ret;
	static uint8_t buffer[1024];
	struct diskio_device_info *info = 0;
	struct FAT_Info fat;
	static int fd;
	static Event_OperationFinished *event;
	i = (uint16_t) data;
	for(i = 0; i < 3; i++) {
		_delay_ms(1000);
		watchdog_periodic();
	}
	rtimer_arch_init();

	redetect:
	while((i = diskio_detect_devices()) != DISKIO_SUCCESS);
	printf("\ndiskio_detect_devices done");
	info = diskio_devices();
	for(i = 0; i < DISKIO_MAX_DEVICES; i++) {
		print_device_info( info + i );
		if( (info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION) ) {
			info += i;
			goto run;
		}
	}
	goto redetect;

	run:

	if( fat_mount_device( info ) != 0 ) {
		printf("\n Mount failed: %u!", ret);
	}
	get_fat_info( &fat );
	printf("\nFAT Info");
	printf("\n\t type            = %u", fat.type);
	printf("\n\t BPB_BytesPerSec = %u", fat.BPB_BytesPerSec);
	printf("\n\t BPB_SecPerClus  = %u", fat.BPB_SecPerClus);
	printf("\n\t BPB_RsvdSecCnt  = %u", fat.BPB_RsvdSecCnt);
	printf("\n\t BPB_NumFATs     = %u", fat.BPB_NumFATs);
	printf("\n\t BPB_RootEntCnt  = %u", fat.BPB_RootEntCnt);
	printf("\n\t BPB_TotSec      = %lu", fat.BPB_TotSec);
	printf("\n\t BPB_Media       = %u", fat.BPB_Media);
	printf("\n\t BPB_FATSz       = %lu", fat.BPB_FATSz);
	printf("\n\t BPB_RootClus    = %lu\n\n", fat.BPB_RootClus);
	printf("\n\n");
	prog_start = RTIMER_NOW();
	process_start( &second_process, NULL );
/*	
	ret = ccfs_open("dir1/book.pdf", CFS_READ, &ret);
	printf("\nOpen returned with %d", ret );
	do {
		PROCESS_WAIT_EVENT();
		//printf("\nFirst process got event", ret );
	} while( ev != get_coop_event_id() );
	event = (Event_OperationFinished *) data;
	fd = event->ret_value;
	printf("\nfd = %d", fd );
	
	for( i = 0; i < 5; i++) {
		start1 = RTIMER_NOW();
		ccfs_read( fd, buffer, 1024, &token1 );
			//printf("\ntoken1 = %d", token1);
		start2 = RTIMER_NOW();
		ccfs_read( fd, buffer, 1024, &token2 );
			//printf("\ntoken2 = %d", token2);
		start3 = RTIMER_NOW();
		ccfs_read( fd, buffer, 1024, &token3 );
			//printf("\ntoken3 = %d", token3);
		start4 = RTIMER_NOW();
		ccfs_read( fd, buffer, 1024, &token4 );
			//printf("\ntoken4 = %d", token4);
		begin = 0;
		do {
			PROCESS_WAIT_EVENT();
			end = RTIMER_NOW();
			//printf("\nFirst Process got event");
			if( ev == get_coop_event_id() ) {
				event = (Event_OperationFinished *) data;
				if( event->token == token1 ) {
					count += (end - start1);
					//printf("\nFirst function");
				} else if( event->token == token2 ) {
					count += (end - start2);
					//printf("\nSecond function");
				} else if( event->token == token3 ) {
					count += (end - start3);
					//printf("\nThird function");
				} else if( event->token == token4) {
					count += (end - start4);
					//printf("\nFourth function");
				}
				begin++;
				//printf("\nbegin = %d", begin);
			}
			if( begin == 4 )
				break;
			else
				continue;
		} while( 1 );
	}*/
	prog_start = RTIMER_NOW() - prog_start;
	printf("\nread count: %lu\n", count);
	printf("\nprog time: %lu\n", prog_start);
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
