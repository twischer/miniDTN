#include "logger.h"
#include "contiki.h"
#include "cfs.h"
#include "sd_mount.h"
#include <stdbool.h>
#include <stdio.h>

#if LOG_LEVEL

#define LOG_FILENAME  "log.txt"

process_event_t event_log_data;

char log_buffer[64];
static bool file_opened = false;

PROCESS(logger_process, "Log process");
PROCESS_THREAD(logger_process, ev, data) {
  static int fd;
  PROCESS_BEGIN();

  event_log_data = process_alloc_event();

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == event_log_data || ev == event_mount);

    if ((ev == event_mount) && (*(bool*) data == true)) {
//      fd = cfs_open(LOG_FILENAME, CFS_WRITE);
//      if (fd == 0) {
//        file_opened = true;
//        log_i("Logfile opened\n");
//      } else {
//        log_e("Failed opening logfile\n");
//      }
    } else {

      // printf it
      printf(log_buffer); //printf(data);
      // file it
//      if (file_opened) {
//        cfs_write(fd, log_buffer, sizeof (log_buffer));
//      }
    }
  }

  PROCESS_END();
}


#endif
