/* 
 * File:   config_mapping.h
 * Author: enrico
 *
 * Created on 1. Februar 2013, 10:30
 */

#ifndef CONFIG_MAPPING_H
#define	CONFIG_MAPPING_H

#include "ini_parser.h"
#include "app_config.h"
#include "logger.h"

int handle_boolean(char* val, void* value_p);
int handle_int(char* value, void* value_p);
int handle_char(char* value, void* value_p);

int handle_boolean(char* value, void* value_p) {
  if (strcmp(value, "true") == 0) {
    *((bool*) value_p) = TRUE;
  } else {
    *((bool*) value_p) = FALSE;
  }
  return 0;
}

int handle_int(char* value, void* value_p) {
  *((int*) value_p) = atoi(value);
  return 0;
}

int handle_char(char* value, void* value_p) {
  *((char*) value_p) = value;
  return 0;
}

int handle_processor(char* value, void* value_p) {
  // TODO: implementation
  return 0;
}

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

void handle_new_processor(void) {
  processor_count++;
  log_v("Found new processor\n");
}

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
    {"rate", &handle_int, &system_config.temp.enabled},
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
//
// file mapping
//
static const cfg_file inga_conf_file = {
  .name = "inga.conf",
  .entries = 7,
  .keys =
  {"node", "output", "acc", "gyro", "pressure", "temp", "processor"},
  .groups =
  {&node_group, &output_group, &acc_group, &gyro_group, &pressure_group, &temp_group, &processor_group},
  .handle =
  {NULL, NULL, NULL, NULL, NULL, NULL, &handle_new_processor}
};

#endif	/* CONFIG_MAPPING_H */

