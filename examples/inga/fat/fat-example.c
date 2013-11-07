#include "contiki.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "clock.h"
#include <util/delay.h>

#include "fat/diskio.h"
#include "fat/cfs-fat.h"

struct diskio_device_info *info = 0;
struct FAT_Info fat;

static struct etimer timer;


char message[] = "This message will destroy itself in 5 second";
char filename[] = "inga_fat.txt";

PROCESS(test_process, "Test process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  // Wait a second...
  etimer_set(&timer, CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
  
  int initialized = 0, i;

  diskio_detect_devices();

  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {

    // break if we have found an SD card partition
    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
      info += i;
      initialized = 1;
      break; 
    }
  }

  if (initialized != 1) {
    printf("Error: Initialization failed.\n");
    PROCESS_EXIT();
  }
 
  // uncomment to automatically format volume (ALL DATA WILL GET LOST!)
  //cfs_fat_mkfs(info);

  // mount volume
  uint8_t retval = cfs_fat_mount_device(info);
  if (retval == 1) {
    printf("Error: Boot sector not found\n");
    PROCESS_EXIT();
  } else if (retval == 2) {
    printf("Error: Unsupported FAT type\n");
    PROCESS_EXIT();
  } else {
    printf("FAT volume mounted\n");
  }

  // let us know some infos about our device 
  cfs_fat_get_fat_info( &fat );
  printf("Volume Size: %ld bytes\n", 512UL*fat.BPB_TotSec);
  printf("             %ld sectors\n", fat.BPB_TotSec);

  // select as default device
  diskio_set_default_device(info);

  // ---

  // open file
  int fd = cfs_open(filename, CFS_WRITE);
  if (fd == -1) {
    printf("Error: failed opening file for writing\n");
    PROCESS_EXIT();
  }

  // write message to file
  uint32_t n = cfs_write(fd, message, sizeof(message));
  printf("%ld bytes written to '%s'\n", n, filename);

  // close
  cfs_close(fd);

  // ---

  // open file
  fd = cfs_open(filename, CFS_READ);
  if (fd == -1) {
    printf("Error: failed opening file for reading\n");
    PROCESS_EXIT();
  }

  // read messag from file
  n = cfs_read(fd, message, sizeof(message));
  printf("%ld bytes read from '%s'\n", n, filename);

  cfs_close(fd);

  PROCESS_END();
}

AUTOSTART_PROCESSES(&test_process);
