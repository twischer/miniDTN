/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#define FILES_IN_STORAGE 20
#define FILE_SIZE 100

#include "contiki.h"
#include "test.h"
#include "../test.h"

#include <stdio.h> /* For printf() */
#include "cfs/cfs-coffee.h"

TEST_SUITE("coffee-tests");

/*---------------------------------------------------------------------------*/
PROCESS(coffee_test_process, "Onboard Flash COFFEE Test process");
AUTOSTART_PROCESSES(&coffee_test_process);
/*---------------------------------------------------------------------------*/
struct etimer et;
#if (COFFEE_DEVICE == 6)
#include "fat/diskio.h"           //tested
struct diskio_device_info *info = 0;
#endif

void fail() {
  TEST_FAIL("");
  watchdog_stop();
  while(1) ;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coffee_test_process, ev, data)
{
  int fd_write, n, i;
  static int cnt = 0;
  uint8_t buffer[FILE_SIZE];
  clock_time_t now;
  unsigned short now_fine;
  static uint32_t time_start, time_stop;

  printf("###########################################################\n");

  PROCESS_BEGIN();

  printf("process running\n");

  // wait for 5 sec
  etimer_set(&et, CLOCK_SECOND * 5);
  PROCESS_YIELD_UNTIL(etimer_expired(&et));

#if (COFFEE_DEVICE == 6)
  int initialized = 0, i;

  //--- Detecting devices and partitions
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

  diskio_set_default_device(info);
#endif

  printf("fomartting...\n");
  TEST_EQUALS(cfs_coffee_format(), 0);
  //printf("test starting\n");

  do {
    now_fine = clock_time();
    now = clock_seconds();
  } while (now_fine != clock_time());
  time_start = ((unsigned long)now)*CLOCK_SECOND + now_fine%CLOCK_SECOND;

  while(1) {
    // Shortest possible pause
    PROCESS_PAUSE();

    // Determine the filename
    char b_file[8];
    sprintf(b_file,"%u.b", cnt);

    // Set the file size
    printf("Reserving '%s'...\n", b_file);
    TEST_EQUALS(cfs_coffee_reserve(b_file, FILE_SIZE), 0);

      // And open it
    printf("Opening '%s'...\n", b_file);
    fd_write = cfs_open(b_file, CFS_WRITE);
    TEST_NEQ(fd_write, -1);

    // fill buffer
    for(i=0; i<FILE_SIZE; i++) {
      buffer[i] = cnt % 0xFF;
    }

    // Open was successful, write has to be successful too since the size has been reserved
    printf("Writing'%s'...\n", b_file);
    n = cfs_write(fd_write, buffer, FILE_SIZE);
    cfs_close(fd_write);

    TEST_EQUALS(n, FILE_SIZE);

    printf("%s written\n", b_file);

    if( cnt >= FILES_IN_STORAGE ) {
      int fd_read;

      // Figure out the filename
      char r_file[8];
      sprintf(r_file,"%u.b", cnt - FILES_IN_STORAGE);

      // Open the bundle file
      printf("Reopening '%s'...\n", r_file);
      fd_read = cfs_open(r_file, CFS_READ);
      if(fd_read == -1) {
        // Could not open file
        printf("############# STORAGE: could not open file %s\n", r_file);
        fail();
      }

      memset(buffer, 0, FILE_SIZE);

      // And now read the bundle back from flash
      printf("Reading '%s'...\n", b_file);
      if (cfs_read(fd_read, buffer, FILE_SIZE) == -1){
        printf("############# STORAGE: cfs_read error\n");
        cfs_close(fd_read);
        fail();
      }
      cfs_close(fd_read);

      for(i=0; i<FILE_SIZE; i++) {
        if( buffer[i] != (cnt - FILES_IN_STORAGE) % 0xFF ) {
          printf("############# STORAGE: verify error\n");
          fail();
        }
      }

      if( cfs_remove(r_file) == -1 ) {
        printf("############# STORAGE: unable to remove %s\n", r_file);
        fail();
      }

      printf("%s deleted\n", r_file);
    }

    cnt ++;

    if( cnt >= 10 ) {
      do {
        now_fine = clock_time();
        now = clock_seconds();
      } while (now_fine != clock_time());
      time_stop = ((unsigned long)now)*CLOCK_SECOND + now_fine%CLOCK_SECOND;

      TEST_REPORT("data written", FILE_SIZE*cnt*CLOCK_SECOND, time_stop-time_start, "bytes/s");
      TEST_PASS();
      watchdog_stop();
      while(1);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
