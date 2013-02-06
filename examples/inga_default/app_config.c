#include "app_config.h"
#include "cfs.h"
#include "cfs-fat.h"
#include "watchdog.h"
#include "config_mapping.h"
#include "sd_mount.h"
#include "logger.h"

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
bool cfg_load_ok;

#ifdef APP_CONF_STORE_EEPROM
app_config_t ee_system_config EEMEM;
#endif

int8_t
app_config_load(bool sd_mounted);

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
  log_i("Loading config from EEPROM...\n");
  eeprom_read_block(&system_config, &ee_system_config, sizeof (system_config));
  // check if data is valid
  if (system_config._check_sequence != CHECK_SEQUENCE) {
    log_e("Loading config from EEPROM failed.\n");
    return -1;
  }
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
  log_i("Saving config to EEPROM...\n");
  system_config._check_sequence = CHECK_SEQUENCE;
  eeprom_update_block(&system_config, &ee_system_config, sizeof (system_config));
#endif
}
/*----------------------------------------------------------------------------*/
static int8_t
app_config_load_microSD() {
  int fd;

  // And open it
  fd = cfs_open("inga.cfg", CFS_READ);

  // In case something goes wrong, we cannot save this file
  if (fd == -1) {
    log_i("############# STORAGE: open for read failed\n");
    return -1;
  }

  char buf[MAX_FILE_SIZE];
  int size = cfs_read(fd, buf, 511);

  log_v("actually read: %d\n", size);

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

process_event_t event_config;
/*----------------------------------------------------------------------------*/
PROCESS(config_process, "Mount process");
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(config_process, ev, data) {
  PROCESS_BEGIN();
  log_v("config_process: started\n");

  event_config = process_alloc_event();

  PROCESS_WAIT_EVENT_UNTIL(ev == event_mount);

  if (app_config_load(*(bool*) data) == 0) {
    cfg_load_ok = true;
  } else {
    cfg_load_ok = false;
  }
  process_post(PROCESS_BROADCAST, event_config, &cfg_load_ok);

  PROCESS_END();
}
/*----------------------------------------------------------------------------*/
int8_t
app_config_load(bool sd_mounted) {
  if (sd_mounted) {
    if (app_config_load_microSD() == 0) {
      log_i("Loaded data from microSD card\n");
      app_config_save_internal();
      log_i("Stored to internal data\n");
      process_post(PROCESS_BROADCAST, event_config, NULL);
      return 0;
    }
  }

  log_i("Loading from microSD card failed\n");

  if (app_config_load_internal() == 0) {
    log_i("Loaded config from internal data\n");
    process_post(PROCESS_BROADCAST, event_config, NULL);
    return 0;
  }

  log_w("Loading from internal data failed\n");

  app_config_load_defaults();
  log_i("Defaults loaded\n");

  process_post(PROCESS_BROADCAST, event_config, NULL);
  return -1;
}
/*----------------------------------------------------------------------------*/
#ifdef APP_CONFIG_DEBUG
void
print_config() {
  log_v("\n\nConfig: \n");
  log_v("_check_sequence:  %d\n", system_config._check_sequence);
  log_v("output.sdcard:    %d\n", system_config.output.sdcard);
  log_v("output.usb:       %d\n", system_config.output.usb);
  log_v("output.radio:     %d\n", system_config.output.radio);
  log_v("output.block_size:%d\n", system_config.output.block_size);
  log_v("acc.enabled:      %d\n", system_config.acc.enabled);
  log_v("acc.rate:         %d\n", system_config.acc.rate);
  log_v("acc.g_range:      %d\n", system_config.acc.g_range);
  log_v("gyro.enabled:     %d\n", system_config.gyro.enabled);
  log_v("gyro.rate:        %d\n", system_config.gyro.rate);
  log_v("gyro.dps:         %d\n", system_config.gyro.dps);
  log_v("pressure.enabled: %d\n", system_config.pressure.enabled);
  log_v("pressure.rate:    %d\n", system_config.pressure.rate);
}
#endif
/*----------------------------------------------------------------------------*/
