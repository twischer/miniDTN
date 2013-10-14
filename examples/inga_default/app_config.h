/* 
 * File:   sys_config.h
 * Author: enrico
 *
 * Created on 1. Februar 2013, 10:32
 */

#ifndef SYS_CONFIG_H
#define	SYS_CONFIG_H

/**
 * @addtogroup default_app
 * @{
 */
/** @defgroup sys_config Global system configuration
 * 
 * @{
 */

#ifndef APP_CONFIG_DEBUG
#define APP_CONFIG_DEBUG 1
#endif

#define APP_CONF_STORE_FLASH  0
#define APP_CONF_STORE_EEPROM 1

#include <stdbool.h>
#include "contiki.h"
#if APP_CONF_STORE_EEPROM
#include <avr/eeprom.h>
#endif

/**
 * node settings
 */
typedef struct {
  int id;
} cfg_node_t;

/**
 * output settings
 */
typedef struct {
  bool sdcard;
  bool usb;
  bool radio;
  uint8_t block_size;
} cfg_output_t;

typedef struct {
  bool enabled;
  uint16_t rate;
} cfg_battery_t;

/** 
 * accelerometer seetings
 */
typedef struct {
  bool enabled;
  uint16_t rate;
  uint8_t g_range;
} cfg_acc_t;

/**
 * gyroscope settings
 */
typedef struct {
  bool enabled;
  uint16_t rate;
  int dps;
} cfg_gyro_t;

/**
 * pressure settings
 */
typedef struct {
  bool enabled;
  uint16_t rate;
} cfg_pressure_t;

/**
 * temperature settings
 */
typedef struct {
  bool enabled;
  uint16_t rate;
} cfg_temp_t;

/**
 * main config struct
 */
typedef struct {
  uint16_t _check_sequence;
  uint16_t _mod_date;
  uint16_t _mod_time;
  cfg_node_t node;
  cfg_output_t output;
  cfg_battery_t battery;
  cfg_acc_t acc;
  cfg_gyro_t gyro;
  cfg_pressure_t pressure;
  cfg_temp_t temp;
} app_config_t;

/**
 * global main config struct
 */
extern app_config_t system_config;

#ifdef APP_CONF_STORE_EEPROM
extern app_config_t ee_system_config EEMEM;
#endif

#define MAX_FILE_SIZE   1024

/*
 * Indicates new config data. Can be thrown by processes to notify about 
 * new config file data.
 */
extern process_event_t event_config_update;

extern char app_config_buffer[];


/**
 * Listens for \ref event_config_update events.
 * 
 * Parses config file, sets config datat struct and puts into internal storage
 */
PROCESS_NAME(config_process);

/**
 * Loads config data using the following order.
 * - If a microSD card is present, configuration data is tried to load from
 *   it and updated configuration is stored to internal storage
 * - If no microSD is present or loading from it failed, configuration data
 *   is tried to load from internal storage (EEPROM)
 * - If also loading from internal storage fails, some default values are used.
 * 
 * @note \b IMPORTANT: Configuration from SDcard is only loaded if the configs file modification
 * timestamp is younger than last time!!
 * 
 * @return 0 if loading succeeded, -1 if loading failed
 */
int8_t app_config_init();

/** Applies config data in app_config_buffer.
 * @todo Data Verification?
 */
void app_config_update();

#ifdef APP_CONFIG_DEBUG
/**
 * Prints 
 */
void print_config();
#endif

/** @} */
/** @} */

#endif	/* SYS_CONFIG_H */

