#include "contiki.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "dev/watchdog.h"
#include "clock.h"

#include "fat/diskio.h"           //tested
#include "fat/fat.h"           //tested

#define CHUNK_SIZE 	128
// #define CHUNKS 		156250 // 20M
#define CHUNKS 		7813 // 1M
// #define CHUNKS			125 // 16K
#define LOOPS 		10

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
	uint8_t buffer[CHUNK_SIZE];
	struct diskio_device_info *info = 0;
	struct FAT_Info fat;
	int fd;
	int n;
	int i;
	int h;
	uint32_t size;
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
	cfs_fat_mount_device( info );
	printf("done\n\n");

	cfs_fat_get_fat_info( &fat );
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

		printf("Writing %s...", b_file);

		size = 0;
		for(h=0; h<CHUNKS; h++ ) {
			for(i=0; i<CHUNK_SIZE; i++) {
				buffer[i] = (cnt + i + h) % 0xFF;
			}

			// Open was successful, write has to be successful too since the size has been reserved
			n = cfs_write(fd, buffer, CHUNK_SIZE);

			if( n != CHUNK_SIZE ) {
				printf("############# STORAGE: Only wrote %d bytes, wanted %d\n", n, CHUNK_SIZE);
				fail();
			}

			size += n;

			if( h % 100 == 0 ) {
				printf("%ld b...", size);
			}
		}

		cfs_close(fd);

		printf("\n\t%s written with %ld bytes\n", b_file, size);

		// Determine the filename
		sprintf(b_file,"%u.bbb", cnt);

		// Open the bundle file
		fd = cfs_open(b_file, CFS_READ);
		if(fd == -1) {
			// Could not open file
			printf("############# STORAGE: could not open file %s\n", b_file);
			fail();
		}

		printf("Reading back %s...", b_file);

		size = 0;
		for(h=0; h<CHUNKS; h++) {
			memset(buffer, 0, CHUNK_SIZE);

			// And now read the bundle back from flash
			n = cfs_read(fd, buffer, CHUNK_SIZE);
			if (n == -1){
				printf("############# STORAGE: cfs_read error\n");
				cfs_close(fd);
				fail();
			}

			for(i=0; i<CHUNK_SIZE; i++) {
				if( buffer[i] != (cnt + i + h) % 0xFF ) {
					printf("############# STORAGE: verify error at %ld: %02X != %02X\n", (size + i), buffer[i], (cnt + i + h) % 0xFF);
					fail();
				}
			}

			size += n;

			if( h % 100 == 0 ) {
				printf("%ld b...", size);
			}
		}
		cfs_close(fd);

		if( cfs_remove(b_file) == -1 ) {
			printf("############# STORAGE: unable to remove %s\n", b_file);
			fail();
		}

		printf("\n\t%s deleted\n", b_file);

		cnt ++;
	}

	cfs_fat_umount_device();

	printf("PASS\n");
	watchdog_stop();
	while(1) ;
                
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
