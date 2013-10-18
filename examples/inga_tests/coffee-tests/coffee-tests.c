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

#include <stdio.h> /* For printf() */
#include "cfs/cfs-coffee.h"
/*---------------------------------------------------------------------------*/
PROCESS(coffee_test_process, "Onboard Flash COFFEE Test process");
AUTOSTART_PROCESSES(&coffee_test_process);
/*---------------------------------------------------------------------------*/
struct etimer et;
int cnt = 0;
void fail() {
  TEST_FAIL("");
  watchdog_stop();
  while(1) ;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coffee_test_process, ev, data)
{
  int fd_write, n, i;
  uint8_t buffer[FILE_SIZE];
  clock_time_t now;
  unsigned short now_fine;
  static uint32_t time_start, time_stop;
  PROCESS_BEGIN();

  printf("process running\n");

  etimer_set(&et, CLOCK_SECOND * 5);
  PROCESS_YIELD_UNTIL(etimer_expired(&et));

  cfs_coffee_format();
  printf("test starting\n");

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
    if( cfs_coffee_reserve(b_file, FILE_SIZE) == -1 ) {
      printf("############# STORAGE: unable to reserve %u bytes\n", FILE_SIZE);
      fail();
    }

    // And open it
    fd_write = cfs_open(b_file, CFS_WRITE);

    // In case something goes wrong, we cannot save this file
    if( fd_write == -1 ) {
      printf("############# STORAGE: open for write failed\n");
      fail();
    }

    for(i=0; i<FILE_SIZE; i++) {
      buffer[i] = cnt % 0xFF;
    }

    // Open was successful, write has to be successful too since the size has been reserved
    n = cfs_write(fd_write, buffer, FILE_SIZE);
    cfs_close(fd_write);

    if( n != FILE_SIZE ) {
      printf("############# STORAGE: Only wrote %d bytes, wanted 100\n", n);
      fail();
    }

    printf("%s written\n", b_file);

    if( cnt >= FILES_IN_STORAGE ) {
      int fd_read;

      // Figure out the filename
      char r_file[8];
      sprintf(r_file,"%u.b", cnt - FILES_IN_STORAGE);

      // Open the bundle file
      fd_read = cfs_open(r_file, CFS_READ);
      if(fd_read == -1) {
        // Could not open file
        printf("############# STORAGE: could not open file %s\n", r_file);
        fail();
      }

      memset(buffer, 0, FILE_SIZE);

      // And now read the bundle back from flash
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

    //if( cnt >= 2 * COFFEE_PAGES ) {
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
