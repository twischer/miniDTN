#include "app_config.h"

#ifdef APP_CONFIG_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// this sequence is used to verify the storage location
#define CHECK_SEQUENCE 0x3535


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
int8_t
app_config_load() {
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
void
app_config_save() {
#if APP_CONF_STORE_EEPROM
  PRINTF("Saving config to EEPROM...");
  system_config._check_sequence = CHECK_SEQUENCE;
  eeprom_update_block(&system_config, &ee_system_config, sizeof (system_config));
  PRINTF("done.\n");
#endif
}
/*----------------------------------------------------------------------------*/
void
app_config_load_defaults() {
  system_config = _default_system_config;
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
