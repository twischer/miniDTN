#include "contiki.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "dev/watchdog.h"
#include "clock.h"

#include "fat/diskio.h"           //tested
#include "fat/fat.h"           //tested

#define FILE_SIZE 128
#define LOOPS 1024

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
void fail() {
	printf("FAIL\n");
	watchdog_stop();
	while(1) ;
}
/*---------------------------------------------------------------------------*/
static struct etimer timer;
int cnt = 0;
PROCESS_THREAD(hello_world_process, ev, data)
{
	uint8_t buffer[1024];
	struct diskio_device_info *info = 0;
	struct FAT_Info fat;
	int fd;
	int n;
	int i;
	int initialized = 0;

	PROCESS_BEGIN();

	etimer_set(&timer, CLOCK_SECOND * 2);
	PROCESS_WAIT_UNTIL(etimer_expired(&timer));

	while( !initialized ) {
		printf("Detecting devices and partitions...");
		while((i = diskio_detect_devices()) != DISKIO_SUCCESS) {
			watchdog_periodic();
		}
		printf("done\n\n");

		info = diskio_devices();
		for(i = 0; i < DISKIO_MAX_DEVICES; i++) {
			print_device_info( info + i );
			printf("\n");

			if( (info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION) ) {
				info += i;
				initialized = 1;
				break;
			}
		}
	}

	printf("Mounting device...");
	fat_mount_device( info );
	printf("done\n\n");

	get_fat_info( &fat );
	printf("FAT Info\n");
	printf("\t type            = %u\n", fat.type);
	printf("\t BPB_BytesPerSec = %u\n", fat.BPB_BytesPerSec);
	printf("\t BPB_SecPerClus  = %u\n", fat.BPB_SecPerClus);
	printf("\t BPB_RsvdSecCnt  = %u\n", fat.BPB_RsvdSecCnt);
	printf("\t BPB_NumFATs     = %u\n", fat.BPB_NumFATs);
	printf("\t BPB_RootEntCnt  = %u\n", fat.BPB_RootEntCnt);
	printf("\t BPB_TotSec      = %lu\n", fat.BPB_TotSec);
	printf("\t BPB_Media       = %u\n", fat.BPB_Media);
	printf("\t BPB_FATSz       = %lu\n", fat.BPB_FATSz);
	printf("\t BPB_RootClus    = %lu\n", fat.BPB_RootClus);
	printf("\n");

	printf("Starting test...\n");

	while(cnt < LOOPS) {
		PROCESS_PAUSE();

		// Determine the filename
		char b_file[8];
		sprintf(b_file,"%u.bbb", cnt);

		// And open it
		fd = cfs_open(b_file, CFS_WRITE);

		// In case something goes wrong, we cannot save this file
		if( fd == -1 ) {
			printf("############# STORAGE: open for write failed\n");
			fail();
		}

		for(i=0; i<FILE_SIZE; i++) {
			buffer[i] = (cnt + i) % 0xFF;
		}

		// Open was successful, write has to be successful too since the size has been reserved
		n = cfs_write(fd, buffer, FILE_SIZE);
		cfs_close(fd);

		if( n != FILE_SIZE ) {
			printf("############# STORAGE: Only wrote %d bytes, wanted 100\n", n);
			fail();
		}

		printf("\t%s written\n", b_file);

		// Determine the filename
		sprintf(b_file,"%u.bbb", cnt);

		// Open the bundle file
		fd = cfs_open(b_file, CFS_READ);
		if(fd == -1) {
			// Could not open file
			printf("############# STORAGE: could not open file %s\n", b_file);
			fail();
		}

		memset(buffer, 0, FILE_SIZE);

		// And now read the bundle back from flash
		if (cfs_read(fd, buffer, FILE_SIZE) == -1){
			printf("############# STORAGE: cfs_read error\n");
			cfs_close(fd);
			fail();
		}
		cfs_close(fd);

		for(i=0; i<FILE_SIZE; i++) {
			if( buffer[i] != (cnt + i) % 0xFF ) {
				printf("############# STORAGE: verify error\n");
				fail();
			}
		}

		if( cfs_remove(b_file) == -1 ) {
			printf("############# STORAGE: unable to remove %s\n", b_file);
			fail();
		}

		printf("\t%s deleted\n", b_file);

		cnt ++;
	}

	fat_umount_device();

	printf("PASS\n");
	watchdog_stop();
	while(1) ;
                
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
