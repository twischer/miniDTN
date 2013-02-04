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

/// booelan
#define bool_t int

// node settings

typedef struct cfg_node_s {
  int id;
} cfg_node;

// output settings

typedef struct cfg_output_s {
  int sdcard;
  int usb;
  int radio;
  int block_size;
} cfg_output;

// accelerometer seetings

typedef struct cfg_acc_s {
  bool_t enabled;
  int rate;
  int dps;
} cfg_acc;

// gyroscope settings

typedef struct cfg_gyro_s {
  bool_t enabled;
  int rate;
  int dps;
} cfg_gyro;

// pressure settings

typedef struct cfg_pressure_s {
  bool_t enabled;
  int rate;
} cfg_pressure;

// temperature settings

typedef struct cfg_temp_s {
  bool_t enabled;
  int rate;
} cfg_temp;

// global main config struct

typedef struct config_s {
  cfg_node node;
  cfg_output output;
  cfg_acc acc;
  cfg_gyro gyro;
  cfg_pressure pressure;
  cfg_temp temp;
} config;

// configuration instance
config system_config;

/** @} */
/** @} */

#endif	/* SYS_CONFIG_H */

