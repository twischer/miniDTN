#include "contiki.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "dev/watchdog.h"
#include "clock.h"
#include "sys/test.h"
#include "../sensor-tests.h"
#include "../test.h"

#include "fat/diskio.h"           //tested
#include "fat/cfs-fat.h"           //tested

#define KBYTE 1024

#define DEBUG 0
#if DEBUG
#define PRINTD(...) printf(__VA_ARGS__)
#else
#define PRINTD(...)
#endif

static int write_test_bytes(const char* name, uint32_t size, uint8_t fill_offset);
static int read_test_bytes(const char* name, uint32_t size, uint8_t fill_offset);
static unsigned long get_file_size(const char* name);

static char * test_sdinit_mount();
static char * test_cfs_read_size();
static char * test_cfs_write_small();
static char * test_cfs_write_large();
static char * test_cfs_remove();
static char * test_cfs_overwrite();

static struct etimer timer;
int cnt = 0;

uint8_t buffer[1024];

/*---------------------------------------------------------------------------*/
static char *
test_sdinit_mount()
{
  struct diskio_device_info *info = 0;
  int initialized = 0, i;

  printf("test_sdinit_mount...\n");
  //--- Detecting devices and partitions
  ASSERT("device detect failed", diskio_detect_devices() == DISKIO_SUCCESS);
  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
      info += i;
      initialized = 1;
      break;
    }
  }
  ASSERT("device initialization failed", initialized == 1);
  
  ASSERT("formatting failed", cfs_fat_mkfs(info) == 0);

  //--- Test mounting volume
  ASSERT("mount failed", cfs_fat_mount_device(info) == 0);

  diskio_set_default_device(info);

  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_cfs_read_size()
{
  int fd;
  printf("test_cfs_read_size...\n");
  //--- Tests for correct return value of cfs_read
  ASSERT("writing 1042 test bytes failed", write_test_bytes("test01.tst", 1042UL, 0) == 0);
  ASSERT("failed to open", cfs_open("test01.tst", CFS_READ) == 0);
  ASSERT("cfs_read() != 1024", cfs_read(fd, buffer, 1024) == 1024);
  ASSERT("cfs_read() != 18", cfs_read(fd, buffer, 1024) == 18);
  ASSERT("cfs_read() != 0", cfs_read(fd, buffer, 1024) == 0);
  cfs_close(fd);
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_cfs_write_small()
{
  printf("test_cfs_write_small...\n");
  //--- Tests for 16 bit size limits 
  ASSERT("writing 12345 bytes failed", write_test_bytes("test02.tst", 12345UL, 0) == 0);
  ASSERT("read file size != 12345", get_file_size("test02.tst") == 12345UL);
  ASSERT("file verification error", read_test_bytes("test02.tst", 12345UL, 0) == 0);
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_cfs_write_large()
{
  printf("test_cfs_write_large...\n");
  //--- Tests for 16 bit size limits 
  ASSERT("writing 80808 bytes failed", write_test_bytes("test03.tst", 80808UL, 0) == 0);
  ASSERT("read file size != 80808", get_file_size("test03.tst") == 80808UL);
  ASSERT("file verification error", read_test_bytes("test03.tst", 80808UL, 0) == 0);
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_cfs_remove()
{
  printf("test_cfs_remove...\n");
  //---
  ASSERT("writing 4711 bytes failed", write_test_bytes("test04.tst", 4711UL, 0) == 0);
  ASSERT("remove failed", cfs_remove("test04.tst") == 0);
  ASSERT("still existent", cfs_open("test04.tst", CFS_READ) != 0);
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_cfs_overwrite()
{
  printf("test_cfs_overwrite...\n");
  //--- Tests simple file write
  ASSERT("writing 12345 bytes failed", write_test_bytes("test05.tst", 12345UL, 0) == 0);
  ASSERT("", get_file_size("test05.tst") == 12345UL);
  ASSERT("", read_test_bytes("test05.tst", 12345UL, 0) == 0);

  //--- Tests overwriting of existent file with smaller file
  ASSERT("writing 4242 bytes failed", write_test_bytes("test05.tst", 4242UL, 42) == 0);
  ASSERT("", get_file_size("test05.tst") == 4242UL);
  ASSERT("", read_test_bytes("test05.tst", 4242UL, 42) == 0);
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Writes exactly size bytes of data to file. */
static int
write_test_bytes(const char* name, uint32_t size, uint8_t fill_offset)
{
  uint8_t buffer[KBYTE];
  uint32_t wsize = 0;
  uint32_t to_write = size;
  uint16_t buffer_size, n;
  int fd;

  fd = cfs_open(name, CFS_WRITE);
  if (fd == -1) {
    PRINTD("############# STORAGE: open for write failed\n");
    return -1;
  }

  PRINTD("Starting to write %ld bytes of data\n", to_write);

  do {
    // fill write buffer
    if (to_write / KBYTE > 0) {
      buffer_size = KBYTE;
      to_write -= KBYTE;
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

    // check
    if (n != buffer_size) {
      PRINTD("############# STORAGE: Only wrote %d bytes, wanted %d\n", n, buffer_size);
      return -1;
    }

    wsize += n;

    //    printf(".");
    PRINTD("%ld left\n", to_write);
  } while (to_write > 0);

  PRINTD("Wrote %ld bytes of data\n", wsize);

  cfs_close(fd);
  return 0;
}
/*---------------------------------------------------------------------------*/
static unsigned long
get_file_size(const char* name)
{
  // And open it
  int fd = cfs_open(name, CFS_READ);
  if (fd == -1) {
    PRINTD("############# STORAGE: open for write failed\n");
    return -1;
  }
  // check file size
  unsigned long read_size = cfs_seek(fd, 0L, CFS_SEEK_END) + 1;
  PRINTD("Size of seek is %lu\n", read_size);
  cfs_close(fd);

  return read_size;
}
/*---------------------------------------------------------------------------*/
static int
read_test_bytes(const char* name, uint32_t size, uint8_t fill_offset)
{
  uint8_t buffer[KBYTE];
  uint32_t wsize = 0;
  uint32_t to_read = size;
  uint16_t buffer_size, n;
  int fd;

  fd = cfs_open(name, CFS_READ);
  if (fd == -1) {
    PRINTD("############# STORAGE: open for read failed\n");
    return -1;
  }

  PRINTD("Starting to read %ld bytes of data\n", to_read);

  buffer_size = KBYTE;

  do {
    // fill write buffer
    if (to_read / KBYTE > 0) {
      to_read -= KBYTE;
    } else {
      to_read = 0;
    }

    // write
    n = cfs_read(fd, buffer, buffer_size);
    
    // bytewise check
    uint16_t i;
    for (i = 0; i < n; i++) {
      if (buffer[i] != (i + fill_offset) % 0xFF) {
        PRINTD("missmatch at %d: %c != %c \n", i, buffer[i], (i + fill_offset) % 0xFF);
        return -1;
      };
    }

    wsize += n;

    //    printf(".");
    PRINTD("%ld left\n", to_read);
  } while (n == buffer_size);

  PRINTD("Read %ld bytes of data\n", wsize);

  cfs_close(fd);

  return 0;
}
/*---------------------------------------------------------------------------*/
int
run_tests()
{
  test_sdinit_mount();
  test_cfs_read_size();
  test_cfs_write_small();
  test_cfs_write_large();
  test_cfs_remove();
  test_cfs_overwrite();
  return errors;
}

AUTOSTART_PROCESSES(&test_process);
