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

#include "webserver-nogui.h"

#include "httpd-fs.h"

#if HTTPD_FS_FAT
#include "fat/diskio.h"
#include "fat/cfs-fat.h"
PROCESS(fat_mount_process, "FAT mount process");
static struct etimer timer;
#endif

/*---------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&webserver_nogui_process, &fat_mount_process);
/*---------------------------------------------------------------------------*/


#if HTTPD_FS_FAT
PROCESS_THREAD(fat_mount_process, ev, data)
{
  struct diskio_device_info *info = 0;
  int i;
  int initialized = 0;

  PROCESS_BEGIN();

  etimer_set(&timer, CLOCK_SECOND * 2);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));

  while (!initialized) {
    printf("Detecting devices and partitions...");
    while ((i = diskio_detect_devices()) != DISKIO_SUCCESS) {
      watchdog_periodic();
    }
    printf("done\n\n");

    info = diskio_devices();
    for (i = 0; i < DISKIO_MAX_DEVICES; i++) {

      if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
        info += i;
        initialized = 1;
        break;
      }
    }
  }

  printf("Mounting device...");
  if (cfs_fat_mount_device(info) == 0) {
    printf("done\n\n");
  } else {
    printf("failed\n\n");
  }

  diskio_set_default_device(info);
  
  PROCESS_END();
}
#endif
