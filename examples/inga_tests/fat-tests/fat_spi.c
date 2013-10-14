#include "contiki.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "dev/watchdog.h"
#include "clock.h"

#include "fat/diskio.h"           //tested
#include "fat/cfs-fat.h"           //tested
#include "microSD.h"
#include "mbr.h"

#define FILE_SIZE 128
#define LOOPS 1024

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
void
fail()
{
  PRINTF("FAIL\n");
  watchdog_stop();
  while (1);
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

  DDRA |= (1 << PA0);
  DDRA |= (1 << PA1);
#define TEST0_ON()  (PORTA |= (1 << PA0))
#define TEST0_OFF() (PORTA &= ~(1 << PA0))
#define TEST1_ON()  (PORTA |= (1 << PA1))
#define TEST1_OFF() (PORTA &= ~(1 << PA1))
  TEST0_OFF();
  TEST1_OFF();

  etimer_set(&timer, CLOCK_SECOND * 2);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));



  if (adxl345_init() == 0) {
    PRINTF(":ADXL345   OK\n");
  } else {
    PRINTF(":ADXL345   FAILURE\n");
  }

  //////////////////////////////////// 1. PASS /////////////////////////////////

  //  while (!initialized) {
  PRINTF("Detecting devices and partitions...");
  while ((i = diskio_detect_devices()) != DISKIO_SUCCESS) {
    watchdog_periodic();
  }
  PRINTF("done\n\n");

  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
//    print_device_info(info + i);
    PRINTF("\n");

    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
      PRINTF("MATCH\n");
      info += i;
      //        initialized = 1;
      break;
    }
  }
  //  }

  PRINTF("Mounting device...");
  if (cfs_fat_mount_device(info) == 0) {
    PRINTF("done\n\n");
  } else {
    PRINTF("failed\n\n");
  }

  diskio_set_default_device(info);

  // And open it
  fd = cfs_open("filename.abc", CFS_WRITE);

  // In case something goes wrong, we cannot save this file
  if (fd == -1) {
    PRINTF("############# STORAGE: open for write failed\n");
    fail();
  }

  for (i = 0; i < FILE_SIZE; i++) {
    buffer[i] = (cnt + i) % 0xFF;
  }
  // Open was successful, write has to be successful too since the size has been reserved
  n = cfs_write(fd, buffer, FILE_SIZE);
  cfs_close(fd);

  if (n != FILE_SIZE) {
    PRINTF("############# STORAGE: Only wrote %d bytes, wanted %d\n", n, FILE_SIZE);
    fail();
  }

  PRINTF("\t%s written\n", "filename.abc");

  /////////////////////////////////// TERMINATE ////////////////////////////////
  struct diskio_device_info info_save;
  info_save.type = info->type;
  info_save.number = info->number;
  info_save.partition = info->partition;
  info_save.num_sectors = info->num_sectors;
  info_save.sector_size = info->sector_size;
  info_save.first_sector = info->first_sector;

  PRINTF("\nUnmounting device...");
  cfs_fat_umount_device();
  //  cfs_fat_flush();
  //  sector_buffer_addr = 0;
  PRINTF("done.");

  microSD_switchoff();
  _delay_ms(100);
  microSD_switchon();


  //////////////////////////////////// 2. PASS /////////////////////////////////

  info = &info_save;

  TEST0_ON();
  //  PRINTF("Detecting devices and partitions...");
  //  while ((i = diskio_detect_devices()) != DISKIO_SUCCESS) {
  //    watchdog_periodic();
  //  }
  //  PRINTF("done\n\n");

//  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
//    //    print_device_info(info + i);
//    PRINTF("\n");
//
//    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
//      PRINTF("MATCH\n");
//      info += i;
//      //        initialized = 1;
//      break;
//    }
//  }
  //  }
//  print_device_info(info);

  microSD_init();
//  struct mbr mbr;
//  mbr_init(&mbr);
//  mbr_read(info, &mbr);
  
  
//  PRINTF("Mounting device...");
//  if (cfs_fat_mount_device(info) == 0) {
//    PRINTF("done\n\n");
//  } else {
//    PRINTF("failed\n\n");
//  }

  diskio_set_default_device(info);


  // Open the bundle file
  fd = cfs_open("filename.abc", CFS_READ);
  if (fd == -1) {
    // Could not open file
    PRINTF("############# STORAGE: could not open file %s\n", "filename.abc");
    fail();
  }

  memset(buffer, 0, FILE_SIZE);

  // And now read the bundle back from flash
  if (cfs_read(fd, buffer, FILE_SIZE) == -1) {
    PRINTF("############# STORAGE: cfs_read error\n");
    cfs_close(fd);
    fail();
  }
  cfs_close(fd);

  for (i = 0; i < FILE_SIZE; i++) {
    if (buffer[i] != (cnt + i) % 0xFF) {
      PRINTF("############# STORAGE: verify error\n");
      fail();
    }
  }

  PRINTF("\t%s read\n", "filename.abc");


  if (cfs_remove("filename.abc") == -1) {
    PRINTF("############# STORAGE: unable to remove %s\n", "filename.abc");
    fail();
  }

  PRINTF("\t%s deleted\n", "filename.abc");

  cnt++;

  cfs_fat_umount_device();

  PRINTF("PASS\n");
  watchdog_stop();

  PRINTF("############# END \n");
  while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
