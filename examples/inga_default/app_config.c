#include "app_config.h"
#include "cfs.h"
#include "cfs-fat.h"
#include "watchdog.h"
#include "config_mapping.h"

#ifdef APP_CONFIG_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// this sequence is used to verify the storage location
#define CHECK_SEQUENCE  0x4242
#define MAX_FILE_SIZE   512

// configuration instance with default values
app_config_t system_config;


#ifdef APP_CONF_STORE_EEPROM
app_config_t ee_system_config EEMEM;
#endif

static const app_config_t _default_system_config = {
  ._check_sequence = CHECK_SEQUENCE,
  // node defaults
  .node.id = 42,
  // output defaults
  .output.sdcard = false,
  .output.usb = false,
  .output.radio = false,
  .output.block_size = 0,
  // accelerometer defaults
  .acc.enabled = true,
  .acc.rate = 0,
  .acc.g_range = 0,
  // gyroscope defaults
  .gyro.enabled = true,
  .gyro.rate = 0,
  .gyro.dps = 0,
  // pressure defaults
  .pressure.enabled = true,
  .pressure.rate = 0,
  // temperature defaults
  .temp.enabled = true,
  .temp.rate = 0
};
/*----------------------------------------------------------------------------*/
static int8_t
app_config_load_internal() {
#if APP_CONF_STORE_EEPROM
  PRINTF("Loading config from EEPROM...");
  eeprom_read_block(&system_config, &ee_system_config, sizeof (system_config));
  // check if data is valid
  if (system_config._check_sequence != CHECK_SEQUENCE) {
    PRINTF("failed.\n");
    return -1;
  }
  PRINTF("done.\n");
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
/**
 * Stores config data to EEPROM.
 */
static void
app_config_save_internal() {
#if APP_CONF_STORE_EEPROM
  PRINTF("Saving config to EEPROM...");
  system_config._check_sequence = CHECK_SEQUENCE;
  eeprom_update_block(&system_config, &ee_system_config, sizeof (system_config));
  PRINTF("done.\n");
#endif
}
/*----------------------------------------------------------------------------*/
static int8_t
app_config_load_microSD() {
  struct diskio_device_info *info = 0;
  int fd;
  int i;
  int initialized = 0;
  printf("Detecting devices and partitions...");
  while ((i = diskio_detect_devices()) != DISKIO_SUCCESS) {
    watchdog_periodic();
  }
  printf("done\n\n");

  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
    print_device_info(info + i);
    printf("\n");

    // use first detected partition
    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
      info += i;
      initialized = 1;
      break;
    }
  }

  if (!initialized) {
    printf("No SD card was found\n");
    return -1;
  }

  printf("Mounting device...");
  if (cfs_fat_mount_device(info) == 0) {
    printf("done\n\n");
  } else {
    printf("failed\n\n");
  }

  diskio_set_default_device(info);

  // And open it
  fd = cfs_open("inga.cfg", CFS_READ);

  // In case something goes wrong, we cannot save this file
  if (fd == -1) {
    printf("############# STORAGE: open for read failed\n");
    return -1;
  }

  char buf[MAX_FILE_SIZE];
  int size = cfs_read(fd, buf, 511);

  printf("actually read: %d\n", size);

  parse_ini(buf, size, &inga_conf_file);

  cfs_close(fd);

  return 0;
}
/*----------------------------------------------------------------------------*/
/**
 * Loads default config data as fallback.
 */
static void
app_config_load_defaults() {
  system_config = _default_system_config;
}
/*----------------------------------------------------------------------------*/
int8_t
app_config_load() {
  if (app_config_load_microSD() == 0) {
    printf("Loaded data from microSD card\n");
    app_config_save_internal();
    printf("Stored to internal data\n");
    return 0;
  }

  printf("Loading from microSD card failed\n");

  if (app_config_load_internal() == 0) {
    printf("Loaded config from internal data\n");
    return 0;
  }

  printf("Loading from internal data failed\n");

  app_config_load_defaults();
  printf("Defaults loaded\n");
  return -1;
}
/*----------------------------------------------------------------------------*/
#ifdef APP_CONFIG_DEBUG
void
print_config() {
  printf("\n\nConfig: \n");
  printf("_check_sequence:  %d\n", system_config._check_sequence);
  printf("output.sdcard:    %d\n", system_config.output.sdcard);
  printf("output.usb:       %d\n", system_config.output.usb);
  printf("output.radio:     %d\n", system_config.output.radio);
  printf("output.block_size:%d\n", system_config.output.block_size);
  printf("acc.enabled:      %d\n", system_config.acc.enabled);
  printf("acc.rate:         %d\n", system_config.acc.rate);
  printf("acc.g_range:      %d\n", system_config.acc.g_range);
  printf("gyro.enabled:     %d\n", system_config.gyro.enabled);
  printf("gyro.rate:        %d\n", system_config.gyro.rate);
  printf("gyro.dps:         %d\n", system_config.gyro.dps);
  printf("pressure.enabled: %d\n", system_config.pressure.enabled);
  printf("pressure.rate:    %d\n", system_config.pressure.rate);
}
#endif
/*----------------------------------------------------------------------------*/
