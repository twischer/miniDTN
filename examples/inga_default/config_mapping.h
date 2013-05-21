/* 
 * File:   config_mapping.h
 * Author: enrico
 *
 * Created on 1. Februar 2013, 10:30
 */

#ifndef CONFIG_MAPPING_H
#define	CONFIG_MAPPING_H

#include <stdlib.h>
#include "ini_parser.h"
#include "app_config.h"
#include "logger.h"

int handle_boolean(char* val, void* value_p);
int handle_int(char* value, void* value_p);
int handle_char(char* value, void* value_p);
int handle_processor(char* value, void* value_p);
void handle_new_processor(void);

// operator types

typedef enum {
  gt, lt, eq
} processor_op;

// processor definition

typedef struct {
  char name[20];
  processor_op type;
  int value;
} processor;

static int processor_count;


// node group mapping
static const cfg_group node_group = {
  .name = "node",
  .entries = 1,
  .entry =
  {
    {"id", &handle_boolean, &system_config.node.id}
  }
};
// output group mapping
static const cfg_group output_group = {
  .name = "output",
  .entries = 4,
  .entry =
  {
    {"sdcard", &handle_boolean, &system_config.output.sdcard},
    {"usb", &handle_boolean, &system_config.output.usb},
    {"radio", &handle_boolean, &system_config.output.radio},
    {"block_size", &handle_int, &system_config.output.block_size}
  }
};
// battery sensor group mapping
static const cfg_group battery_group = {
  .name = "battery",
  .entries = 2,
  .entry =
  {
    {"enabled", &handle_boolean, &system_config.battery.enabled},
    {"rate", &handle_int, &system_config.battery.rate}
  }
};
// accelerometer sensor group mapping
static const cfg_group acc_group = {
  .name = "acc",
  .entries = 3,
  .entry =
  {
    {"enabled", &handle_boolean, &system_config.acc.enabled},
    {"rate", &handle_int, &system_config.acc.rate},
    {"g_range", &handle_int, &system_config.acc.g_range},
  }
};
// gyroscope sensor group mapping
static const cfg_group gyro_group = {
  .name = "gyro",
  .entries = 3,
  .entry =
  {
    {"enabled", &handle_boolean, &system_config.gyro.enabled},
    {"rate", &handle_int, &system_config.gyro.rate},
    {"dps", &handle_int, &system_config.gyro.dps}
  }
};
// pressure sensor group mapping
static const cfg_group pressure_group = {
  .name = "pressure",
  .entries = 2,
  .entry =
  {
    {"enabled", &handle_boolean, &system_config.pressure.enabled},
    {"rate", &handle_int, &system_config.pressure.rate},
  }
};
// temperature sensor group mapping
static const cfg_group temp_group = {
  .name = "temp",
  .entries = 2,
  .entry =
  {
    {"enabled", &handle_boolean, &system_config.temp.enabled},
    {"rate", &handle_int, &system_config.temp.rate},
  }
};
// processor group mapping
static const cfg_group processor_group = {
  .name = "processor",
  .entries = 3,
  .entry =
  {
    // name of processor (user defined)
    {"name", &handle_processor, NULL},
    // operation to perform
    {"op", &handle_processor, NULL},
    // value used for operation
    {"value", &handle_processor, NULL},
  }
};

extern const cfg_file inga_conf_file;

#endif	/* CONFIG_MAPPING_H */

