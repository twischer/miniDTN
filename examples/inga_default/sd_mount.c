#include "sd_mount.h"

#include "logger.h"
#include "cfs.h"
#include "cfs-fat.h"
#include "watchdog.h"
#include <stdbool.h>
#include "default_app.h"

process_event_t event_mount;

static int8_t sd_mount();

PROCESS(mount_process, "Mount process");
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(mount_process, ev, data)
{
  PROCESS_BEGIN();
  log_v("mount_process: started\n");

  event_mount = process_alloc_event();

  if (sd_mount() == 0) {
    // mark sd as mounted (system wide)
    sysflag_set_en(SYS_SD_MOUNTED);
  } else {
    // mark sd as not mounted (system wide)
    sysflag_set_dis(SYS_SD_MOUNTED);
  }
  process_post(PROCESS_BROADCAST, event_mount, NULL);

  PROCESS_END();
}
/*----------------------------------------------------------------------------*/
static int8_t
sd_mount()
{
  struct diskio_device_info *info = 0;
  int i;
  int initialized = 0;
  log_i("Detecting devices and partitions...\n");
  while ((i = diskio_detect_devices()) != DISKIO_SUCCESS) {
    watchdog_periodic();
  }

  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
    log_i(" ");
    print_device_info(info + i);

    // use first detected partition
    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
      info += i;
      initialized = 1;
      break;
    }
  }

  if (!initialized) {
    log_w("No SD card was found\n");
    return -1;
  }

  log_i("Mounting device...\n");
  if (cfs_fat_mount_device(info) == 0) {
    diskio_set_default_device(info);
    return 0;
  } else {
    log_e("Mounting device failed\n");
    return -1;
  }

}
/*----------------------------------------------------------------------------*/
