#include "contiki.h"
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "dev/watchdog.h"
#include "clock.h"

#include "fat/diskio.h"           //tested
#include "fat/cfs-fat.h"           //tested

//#define CHUNK_SIZE 	128
#define KBYTE 1024
#define CHUNK_SIZE  8192
#define CHUNKS 100
// #define CHUNKS 		156250 // 20M
//#define CHUNKS 		7813 // 1M
// #define CHUNKS			125 // 16K
#define LOOPS 		1

static void write_test_bytes(const char* name, uint32_t size, uint8_t fill_offset);
static unsigned long  get_file_size(const char* name);

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
void
success()
{
  printf("SUCCESS\n");
}
/*---------------------------------------------------------------------------*/
#define ASSERT(cond) if (cond) { success();} else { printf("cond "); fail(); }
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
  int h;
  uint32_t size;
  int initialized = 0;

  PROCESS_BEGIN();

  etimer_set(&timer, CLOCK_SECOND * 2);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));

  //	while( !initialized ) {
  printf("Detecting devices and partitions...");
  while ((i = diskio_detect_devices()) != DISKIO_SUCCESS) {
    watchdog_periodic();
  }
  printf("done\n\n");

  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
    print_device_info(info + i);
    printf("\n");

    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
      info += i;
      initialized = 1;
      break;
    }
  }
  //	}

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

    //--- Tests simple file write
    write_test_bytes("test01.tst", 12345UL, 0);
    ASSERT(get_file_size("test01.tst") == 12345UL);

    //--- Tests overwriting of existend file
    write_test_bytes("test01.tst", 4242UL, 42);
    ASSERT(get_file_size("test01.tst") == 4242UL);

    //--- Tests for 16 bit size limits 
    write_test_bytes("test02.tst", 80808UL, 0);
    ASSERT(get_file_size("test02.tst") == 80808UL);
    
//    // Determine the filename
//    sprintf(b_file, "%u.bbb", cnt);
//
//    // Open the bundle file
//    fd = cfs_open(b_file, CFS_READ);
//    if (fd == -1) {
//      // Could not open file
//      printf("############# STORAGE: could not open file %s\n", b_file);
//      fail();
//    }


    cfs_fat_umount_device();
    return 0;


//    printf("Reading back %s...", b_file);

    size = 0;
    for (h = 0; h < CHUNKS; h++) {
      memset(buffer, 0, CHUNK_SIZE);

      // And now read the bundle back from flash
      n = cfs_read(fd, buffer, CHUNK_SIZE);
      if (n == -1) {
        printf("############# STORAGE: cfs_read error\n");
        cfs_close(fd);
        fail();
      }

      for (i = 0; i < CHUNK_SIZE; i++) {
        if (buffer[i] != (cnt + i + h) % 0xFF) {
          printf("############# STORAGE: verify error at %ld: %02X != %02X\n", (size + i), buffer[i], (cnt + i + h) % 0xFF);
          fail();
        }
      }

      size += n;

      if (h % 100 == 0) {
        printf("%ld b...", size);
      }
    }
    cfs_close(fd);

    //		if( cfs_remove(b_file) == -1 ) {
    //			printf("############# STORAGE: unable to remove %s\n", b_file);
    //			fail();
    //		}
    //
    //		printf("\n\t%s deleted\n", b_file);

    cnt++;
  }

  cfs_fat_umount_device();

  printf("PASS\n");
//  printf("####################################################\n");

  watchdog_stop();
  while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/* Writes exactly size bytes of data to file. */
static void
write_test_bytes(const char* name, uint32_t size, uint8_t fill_offset)
{
  uint8_t buffer[KBYTE];
  uint32_t wsize = 0;
  uint32_t to_write = size;
  uint16_t buffer_size, n;
  int fd;

  fd = cfs_open(name, CFS_WRITE);
  if (fd == -1) {
    printf("############# STORAGE: open for write failed\n");
    fail();
  }

  printf("Starting to write %ld bytes of data\n", to_write);

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
      printf("############# STORAGE: Only wrote %d bytes, wanted %d\n", n, buffer_size);
      //				fail();
    }

    wsize += n;

    printf("%ld left\n", to_write);
  } while (to_write > 0);

  printf("Wrote %ld bytes of data\n", wsize);

  cfs_close(fd);
}
/*---------------------------------------------------------------------------*/
static unsigned long 
get_file_size(const char* name)
{
  // And open it
  int fd = cfs_open(name, CFS_READ);
  if (fd == -1) {
    printf("############# STORAGE: open for write failed\n");
    fail();
  }
  // check file size
  unsigned long read_size = cfs_seek(fd, 0L, CFS_SEEK_END) + 1;
  printf("Size of seek is %lu\n", read_size);
  cfs_close(fd);
  
  return read_size;
}
