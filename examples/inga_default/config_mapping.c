#include "config_mapping.h"

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
  *((char*) value_p) = *value;
  return 0;
}

int handle_processor(char* value, void* value_p) {
  // TODO: implementation
  return 0;
}
void handle_new_processor(void) {
  processor_count++;
  log_v("Found new processor\n");
}


//
// file mapping
//
const cfg_file inga_conf_file = {
  .name = "inga.conf",
  .entries = 7,
  .keys =
  {"node", "output", "battery", "acc", "gyro", "pressure", "temp", "processor"},
  .groups =
  {&node_group, &output_group, &battery_group, &acc_group, &gyro_group, &pressure_group, &temp_group, &processor_group},
  .handle =
  {NULL, NULL, NULL, NULL, NULL, NULL, &handle_new_processor}
};
