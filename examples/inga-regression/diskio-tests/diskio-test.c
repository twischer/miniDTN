#include "contiki.h"
#ifdef INGA_REVISION
#include <util/delay.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#define DEBUG                                   0

#include "fat/diskio.h"
#include <stdio.h>
#include <avr/wdt.h>
#include "../test.h"

#define BLOCKS_TO_WRITE 512

#ifdef DISKIO_TEST_SD
#define DISKIO_TEST_DEVICE  (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)
#elif defined DISKIO_TEST_EXTFLASH
#define DISKIO_TEST_DEVICE  DISKIO_DEVICE_TYPE_GENERIC_FLASH
#else
#error Neither DISKIO_TEST_SD nor DISKIO_TEST_EXTFLASH defined
#endif

#define FPI 0

// fancy progress indicator
#if FPI
#define FANCY_PROGRESS() printf(".")
#else
#define FANCY_PROGRESS()
#endif

TEST_SUITE("diskio-test");

/*---------------------------------------------------------------------------*/
PROCESS(diskio_test_process, "diskio test process");
AUTOSTART_PROCESSES(&diskio_test_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(diskio_test_process, ev, data)
{
  PROCESS_BEGIN();
  uint16_t i = 0;
  int retcode;
  uint8_t buffer[512];
  struct diskio_device_info *info = 0;
  uint32_t exectime;
  
#ifdef DISKIO_TEST_SD
  SDCARD_POWER_ON();
#endif

  TEST_EQUALS(diskio_detect_devices(), DISKIO_SUCCESS);

  info = diskio_devices();

  // display all found devices
  //while ((info + i)->type != 0) {
  //  diskio_print_device_info( info + i );
  //  i++;
  //} 

  i = 0;
  while ((info + i)->type != 0) {
    if( (info + i)->type == DISKIO_TEST_DEVICE ) {
      info += i;
      break;
    }
    i++;
  }

  //printf("\nWill write on:\n");
  //diskio_print_device_info(info);

  for (i = 0; i < 512; i++) {
    buffer[i] = i;
  }

  exectime = RTIMER_NOW();
  for(i = 0; i < BLOCKS_TO_WRITE; i++) {
    FANCY_PROGRESS();
    retcode = diskio_write_block( info, i, buffer );
    watchdog_periodic();
    TEST_EQUALS(retcode, DISKIO_SUCCESS);
    //if (retcode != DISKIO_SUCCESS) {
    //  printf("Error: retcode %d\n", retcode);
    //}
  }
  exectime = RTIMER_NOW() - exectime;
  printf("\n");
  //printf("Write Time (1024 blocks): %lu\n", exectime*1000 / RTIMER_SECOND);
  TEST_REPORT("Write speed", 512*BLOCKS_TO_WRITE*RTIMER_SECOND, exectime*1024, "kbyte/second");

  // ---

  exectime = RTIMER_NOW();
  for(i = 0; i < BLOCKS_TO_WRITE; i++) {
    FANCY_PROGRESS();
    memset(buffer, 0, 512);
    retcode = diskio_read_block( info, i, buffer ); 
    watchdog_periodic();
    TEST_EQUALS(retcode, DISKIO_SUCCESS);
    //if (retcode != DISKIO_SUCCESS) {
    //  printf("Error: retcode %d\n", retcode);
    //}
    uint16_t j;
    for (j = 0; j < 512; j++) {
      //printf("%d, %d\n", buffer[j], j & 0xFF);
      if (buffer[j] != (j & 0xFF)) {
        //printf("Error: mismatch in block %d: %u != %u\n", i, buffer[j], j);
        TEST_EQUALS(buffer[j], j);
        break;
      }
    }
  }
  exectime = RTIMER_NOW() - exectime;
  printf("\n");
  //printf("Read Time (1024 blocks): %lu\n", exectime*1000 / RTIMER_SECOND);
  TEST_REPORT("Read speed", 512*BLOCKS_TO_WRITE*RTIMER_SECOND, exectime*1024, "kbyte/second");

  TESTS_DONE();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
