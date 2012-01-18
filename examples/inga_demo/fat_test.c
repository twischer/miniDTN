
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
#include "drv/fat/fat.h"           //tested
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
	struct diskio_device_info *info = 0;
	struct FAT_Info fat;
	wdt_disable();
	printf("\nTEST BEGIN\n");
	while(diskio_detect_devices() != DISKIO_SUCCESS);
	info = diskio_devices();
	for(i = 0; i < DISKIO_MAX_DEVICES; i++) {
		print_device_info( info + i );
		if( (info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION) ) {
			info += i;
			break;
		}
	}
	printf("\nfat_mount_device() = %u", fat_mount_device( info ) );
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
	printf("\n\t BPB_RootClus    = %lu", fat.BPB_RootClus);
	printf("\n");
                
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
