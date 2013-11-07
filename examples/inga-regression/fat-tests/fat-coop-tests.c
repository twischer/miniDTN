#include "contiki.h"
#include <stdio.h>
#include <stdlib.h>
#include "leds.h"

#include "fat/diskio.h"           //tested
#include "fat/fat_coop.h"           //tested
#include "dev/watchdog.h"
#include "clock.h"

#include "../test.h"

#define FILE_SIZE 128
#define LOOPS 1024

TEST_SUITE("fat-coop-test");

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
void
fail()
{
  printf("FAIL\n");
  watchdog_stop();
  while (1);
}
/*---------------------------------------------------------------------------*/
static struct etimer timer;
int cnt = 0;
int fd;
process_event_t coop_event;
char b_file[8];
uint8_t buffer[256];
uint8_t token = 0;
Event_OperationFinished * event;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  struct diskio_device_info *info = 0;
  struct FAT_Info fat;
  int n;
  int i;
  int initialized = 0;


  PROCESS_BEGIN();

  etimer_set(&timer, CLOCK_SECOND * 2);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));

  TEST_EQUALS(diskio_detect_devices(), DISKIO_SUCCESS);

  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
      info += i;
      initialized = 1;
      break; 
    }
  }
  TEST_EQUALS(initialized, 1);

  printf("Formatting device...");
  cfs_fat_mkfs(info);
  printf("done\n\n");
  
  printf("Mounting device...");
  cfs_fat_mount_device(info);
  printf("done\n\n");

  cfs_fat_get_fat_info(&fat);
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
  
  while (cnt < LOOPS) {
    PROCESS_PAUSE();

    // Determine the filename
    sprintf(b_file, "%05u.b", cnt);

    printf("Opening file...\n");

    // Open the file descriptor
    fd = ccfs_open(b_file, CFS_WRITE, &token);
    // In case something goes wrong, we cannot save this file
    if (fd != 0) {
      printf("############# STORAGE: ccfs_open for write failed\n");
      fail();
    }

    // Wait until the file is open
    coop_event = get_coop_event_id();
    PROCESS_WAIT_UNTIL(ev == coop_event && ((Event_OperationFinished *) data)->token == token);
    event = (Event_OperationFinished *) data;

    printf("File opened...\n");

    // In case something goes wrong, we cannot save this file
    if (event->ret_value == -1) {
      printf("############# STORAGE: open for write failed\n");
      fail();
    }

    // Fill the buffer
    for (i = 0; i < FILE_SIZE; i++) {
      buffer[i] = (cnt + i) % 0xFF;
    }

    printf("Writing file...\n");

    // Write to file
    n = ccfs_write(fd, buffer, FILE_SIZE, &token);

    if (n != 0) {
      printf("############# STORAGE: Write failed\n");
      fail();
    }

    // Wait until write is finished
    PROCESS_WAIT_UNTIL(ev == coop_event && ((Event_OperationFinished *) data)->token == token);
    event = (Event_OperationFinished *) data;

    printf("File written...\n");

    if (event->ret_value != FILE_SIZE) {
      printf("############# STORAGE: Write failed and returned %d\n", event->ret_value);
      fail();
    }

    // Close the file
    n = ccfs_close(fd, &token);
    if (n != 0) {
      printf("############# STORAGE: Close failed\n");

    }

    // Wait until close is finished
    PROCESS_WAIT_UNTIL(ev == coop_event && ((Event_OperationFinished *) data)->token == token);
    event = (Event_OperationFinished *) data;

    printf("\n\t%s written\n", b_file);

    // Open the file for reading
    fd = ccfs_open(b_file, CFS_READ, &token);
    // In case something goes wrong, we cannot save this file
    if (fd != 0) {
      printf("############# STORAGE: open for read failed\n");
      fail();
    }

    // Wait until finished
    PROCESS_WAIT_UNTIL(ev == coop_event && ((Event_OperationFinished *) data)->token == token);
    event = (Event_OperationFinished *) data;

    // In case something goes wrong, we cannot save this file
    if (event->ret_value == -1) {
      printf("############# STORAGE: open for read failed\n");
      cfs_fat_print_file_info(fd);
      fail();
    }

    memset(buffer, 0, FILE_SIZE);

    // And now read the bundle back from flash
    n = ccfs_read(fd, buffer, FILE_SIZE, &token);

    if (n != 0) {
      printf("############# STORAGE: ccfs_read error\n");
      fail();
    }

    // Wait until read is finished
    PROCESS_WAIT_UNTIL(ev == coop_event && ((Event_OperationFinished *) data)->token == token);
    event = (Event_OperationFinished *) data;

    if (event->ret_value != FILE_SIZE) {
      printf("############# STORAGE: read failed and returned %d\n", event->ret_value);
      fail();
    }

    // Close file
    n = ccfs_close(fd, &token);
    if (n != 0) {
      printf("############# STORAGE: Close failed\n");

    }

    // Wait
    PROCESS_WAIT_UNTIL(ev == coop_event && ((Event_OperationFinished *) data)->token == token);
    event = (Event_OperationFinished *) data;

    // Verify contents
    for (i = 0; i < FILE_SIZE; i++) {
      if (buffer[i] != (cnt + i) % 0xFF) {
        printf("############# STORAGE: verify error at %d: %02X != %02X\n", i, buffer[i], (cnt + i) % 0xFF);
        fail();
      }
    }

    //		n = ccfs_remove(b_file, &token);
    //
    //		if( n != 0 ) {
    //			printf("############# STORAGE: unable to remove %s\n", b_file);
    //			fail();
    //		}
    //
    //		PROCESS_WAIT_UNTIL(ev == coop_event && ((Event_OperationFinished *) data)->token == token);
    //		event = (Event_OperationFinished *) data;
    //
    //		printf("\t%s deleted\n", b_file);

    printf("\n");

    cnt++;
  }

  cfs_fat_umount_device();

  TESTS_DONE();

  watchdog_stop();
  while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
