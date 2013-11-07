#include "contiki.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "dev/watchdog.h"
#include "clock.h"
#include "../test.h"
#include <util/delay.h>

#include "fat/diskio.h"           //tested
#include "fat/cfs-fat.h"           //tested

/*--- Test parameters ---*/
#define FAT_TEST_CONF_NUM_FILES     15
#define FAT_TEST_CONF_BUF_SIZE      1024
/*--- ---*/
uint32_t file_sizes[FAT_TEST_CONF_NUM_FILES] = {127, 128, 511, 512, 1023, 1024, 4095, 4096, 12345, 16383, 16384, 32767, 32768U, 65535UL, 65536UL};
uint8_t file_inits[FAT_TEST_CONF_NUM_FILES] = {5, 12, 80, 3, 76, 13, 123, 42, 23, 200, 255, 7, 99, 12, 77, 31};

#ifdef FAT_TEST_SD
#define FAT_TEST_DEVICE (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)
#endif
#ifdef FAT_TEST_EXTFLASH
#define FAT_TEST_DEVICE DISKIO_DEVICE_TYPE_GENERIC_FLASH  
#endif

#define DEBUG 0
#if DEBUG
#define PRINTD(...) printf(__VA_ARGS__)
#else
#define PRINTD(...)
#endif

void write_test_bytes(const char* name, uint32_t size, uint8_t fill_offset);
void read_test_bytes(const char* name, uint32_t size, uint8_t fill_offset);
static unsigned long get_file_size(const char* name);

static struct etimer timer;
int cnt = 0;

TEST_SUITE("fat-test");

struct diskio_device_info *info = 0;

/*---------------------------------------------------------------------------*/
void
test_device_init()
{
  int initialized = 0, i;

  //--- Detecting devices and partitions
  TEST_EQUALS(diskio_detect_devices(), DISKIO_SUCCESS);

  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
    printf("info + 1 type: %d\n", (info + i)->type);
    if ((info + i)->type == FAT_TEST_DEVICE) {
      info += i;
      initialized = 1;
      break; 
    }
  }
  TEST_EQUALS(initialized, 1);
 
}
/*---------------------------------------------------------------------------*/
void
test_mkfs()
{
  struct FAT_Info fat;

  TEST_CODE();

  // format partition
  TEST_EQUALS(cfs_fat_mkfs(info), 0);
  // mount volume
  TEST_EQUALS(cfs_fat_mount_device(info), 0);

  TEST_POST();

  cfs_fat_get_fat_info( &fat );

  printf("\n");
  TEST_REPORT("Capacity", 512*fat.BPB_TotSec, 1073741824, "GB");
  // Must be one of 512, 1024, 2048, 4096.
  TEST_REPORT("BytesPerSec", fat.BPB_BytesPerSec, 1, "bytes/sector");
  TEST_REPORT("Reserved sectors", fat.BPB_RsvdSecCnt, 1, "sectors");
  TEST_REPORT("Total sectors", fat.BPB_TotSec, 1, "sectors");
  TEST_REPORT("Num of FATs", fat.BPB_NumFATs, 1, "");
  // Must be one of 1, 2, 4, 8, 16, 32, 64, 128.
  TEST_REPORT("SecPerClus", fat.BPB_SecPerClus, 1, "sectors");

  //TEST_EQUALS(fat.BPB_BytesPerSec, 512);
  //TEST_EQUALS(fat.BPB_SecPerClus, 8);
  TEST_EQUALS(fat.BPB_NumFATs, 2);
  TEST_EQUALS(fat.BPB_Media, 0xF8);

  diskio_set_default_device(info);
}
/*---------------------------------------------------------------------------*/

void
test_cfs_write_files()
{
  int idx;
  char fnamebuf[12];

  // Write test files....
  for (idx = 0; idx < FAT_TEST_CONF_NUM_FILES; idx++) {
    sprintf(fnamebuf, "test%02d.dat", idx);
    write_test_bytes(fnamebuf, file_sizes[idx], file_inits[idx]);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
test_cfs_seek()
{
  int idx;
  char fnamebuf[12];
  // Test file size
  for (idx = 0; idx < FAT_TEST_CONF_NUM_FILES; idx++) {
    sprintf(fnamebuf, "test%02d.dat", idx);
    TEST_EQUALS(get_file_size(fnamebuf), file_sizes[idx]);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
test_cfs_read_files()
{
  int idx;
  char fnamebuf[12];

  // Test file content
  for (idx = 0; idx < FAT_TEST_CONF_NUM_FILES; idx++) {
    sprintf(fnamebuf, "test%02d.dat", idx);
    read_test_bytes(fnamebuf, file_sizes[idx], file_inits[idx]);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
test_cfs_remove()
{
  int idx;
  char fnamebuf[12];

  // Test file remove
  for (idx = 0; idx < FAT_TEST_CONF_NUM_FILES; idx++) {
    sprintf(fnamebuf, "test%02d.dat", idx);

    TEST_CODE();

    TEST_EQUALS(cfs_remove(fnamebuf), 0);

    TEST_POST();

    TEST_NEQ(cfs_open(fnamebuf, CFS_READ), 0);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
/* Writes exactly size bytes of data to file. */
void
write_test_bytes(const char* name, uint32_t size, uint8_t fill_offset)
{
  uint8_t buffer[FAT_TEST_CONF_BUF_SIZE];
  uint32_t wsize = 0;
  uint32_t to_write = size;
  uint16_t buffer_size, n;
  int fd;

  fd = cfs_open(name, CFS_WRITE);
  TEST_NEQ(fd, -1);

  PRINTD("Starting to write %ld bytes of data\n", to_write);

  do {
    // fill write buffer
    if (to_write / FAT_TEST_CONF_BUF_SIZE > 0) {
      buffer_size = FAT_TEST_CONF_BUF_SIZE;
      to_write -= FAT_TEST_CONF_BUF_SIZE;
    } else {
      buffer_size = to_write;
      to_write = 0;
    }
    uint16_t i;
    for (i = 0; i < buffer_size; i++) {
      buffer[i] = (i + fill_offset) % 0xFF;
    }

    // write
    n = cfs_write(fd, buffer, buffer_size);
    TEST_EQUALS(n, buffer_size);

    // check
    TEST_EQUALS(n, buffer_size);

    wsize += n;

    PRINTD("%ld left\n", to_write);
  } while (to_write > 0);

  TEST_EQUALS(wsize, size);

  TEST_REPORT("Bytes written to file", wsize, 1, "bytes");

  cfs_close(fd);
}
/*---------------------------------------------------------------------------*/
static unsigned long
get_file_size(const char* name)
{
  uint8_t buffer[FAT_TEST_CONF_BUF_SIZE];
  // And open it
  int fd = cfs_open(name, CFS_READ);
  TEST_NEQ(fd, -1);

  // NOTE: we need a delay here, otherwise test will fail!
  // Reason might be sd card access timing 
  // @TODO: check this!
  _delay_ms(10);

  // check file size
  unsigned long read_size = cfs_seek(fd, 0L, CFS_SEEK_END) + 1;
  TEST_REPORT("Size of seek", read_size, 1, "bytes");
  cfs_close(fd);

  return read_size;
}
/*---------------------------------------------------------------------------*/
void
read_test_bytes(const char* name, uint32_t size, uint8_t fill_offset)
{
  uint8_t buffer[FAT_TEST_CONF_BUF_SIZE];
  uint32_t wsize = 0;
  uint32_t to_read = size;
  uint32_t read_size;
  uint16_t buffer_size, n;
  int fd;

  fd = cfs_open(name, CFS_READ);
  TEST_NEQ(fd, -1);

  PRINTD("Starting to read %ld bytes of data\n", to_read);

  buffer_size = FAT_TEST_CONF_BUF_SIZE;

  do {
    //
    if (to_read / FAT_TEST_CONF_BUF_SIZE > 0) {
      read_size = FAT_TEST_CONF_BUF_SIZE;
      to_read -= FAT_TEST_CONF_BUF_SIZE;
    } else {
      read_size = to_read;
      to_read = 0;
    }

    // read
    n = cfs_read(fd, buffer, buffer_size);
    TEST_EQUALS(n, read_size);
    
    // bytewise check
    uint16_t i;
    for (i = 0; i < n; i++) {
      TEST_EQUALS(buffer[i], (i + fill_offset) % 0xFF);
    }

    wsize += n;

    //    printf(".");
    PRINTD("%ld left\n", to_read);
  } while (n == buffer_size);

  TEST_REPORT("Bytes read from file", wsize, 1, "bytes");

  cfs_close(fd);
}
static struct etimer timer;

PROCESS(test_process, "Test process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  // Wait a second...
  etimer_set(&timer, CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
  
  RUN_TEST("test_device_init", test_device_init);
  RUN_TEST("test_mkfs", test_mkfs);
  RUN_TEST("cfs_write_files", test_cfs_write_files);
  RUN_TEST("cfs_seek", test_cfs_seek);
  RUN_TEST("cfs_read_files", test_cfs_read_files);
  RUN_TEST("test_cfs_remove", test_cfs_remove);
  
  TESTS_DONE();

  PROCESS_END();
}


AUTOSTART_PROCESSES(&test_process);
