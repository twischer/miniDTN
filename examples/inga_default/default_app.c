/**
 * default app.c
 */
#include "contiki.h"

#include <util/delay.h>
#include <stdlib.h>

#include <avr/io.h>

#define DEBUG                                   1

// standard includes
#include <stdio.h> /* For printf() */
#include <stdint.h>
// contiki includes
#include "acc-sensor.h"
#include "adc-sensor.h"
#include "button-sensor.h"
#include "gyro-sensor.h"
#include "pressure-sensor.h"
#include "radio-sensor.h"
// app includes
#include "ini_parser.h"
#include "app_config.h"
#include "logger.h"
#include "sd_mount.h"
#include "uart_handler.h"

/*---------------------------------------------------------------------------*/
PROCESS(default_app_process, "Sensor update process");
AUTOSTART_PROCESSES(&default_app_process, &logger_process, &config_process, &mount_process); //, &mount_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(default_app_process, ev, data)
{
  PROCESS_BEGIN();

  uart_handler_init();

  // we wait until mounted, before loading config
  PROCESS_WAIT_EVENT_UNTIL(ev == event_mount);

  if (app_config_init(*(bool*) data) != 0) {
    log_e("Loading config failed!\n");
  }

  print_config();

  // sensor setup
  if (system_config.acc.enabled) {
    log_i("Enabling accelerometer\n");
    SENSORS_ACTIVATE(acc_sensor);
    // set range
    if (!acc_sensor.configure(ACC_CONF_SENSITIVITY, system_config.acc.g_range)) {
      log_e("Failed setting g range\n");
    }
    if (!acc_sensor.configure(ACC_CONF_DATA_RATE, system_config.acc.rate)) {
      log_e("Failed setting data rate\n");
    }
  }
  if (system_config.gyro.enabled) {
    log_i("Enabling gyroscope\n");
    SENSORS_ACTIVATE(gyro_sensor);
  }
  if (system_config.pressure.enabled || system_config.temp.enabled) {
    log_i("Enabling pressure/temperature sensor\n");
    SENSORS_ACTIVATE(pressure_sensor);
  }

  //  system_config.pressure.

  etimer_set(&timer, CLOCK_SECOND * 0.05);

//  printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
  //  int idx;
  //  for (idx = 0; idx < 1024; idx++) {
  //    ramData[idx] = 0x42;
  //  }
  //  eeprom_write_block(&ramData, &eeData, sizeof (ramData));
  //  while (1) {
  //
  //    PROCESS_YIELD();
  //    etimer_set(&timer, CLOCK_SECOND * 0.05);
  //    printf("X_ACC=%d, Y_ACC=%d, Z_ACC=%d\n",
  //            acc_sensor.value(ACC_X),
  //            acc_sensor.value(ACC_Y),
  //            acc_sensor.value(ACC_Z));
  //    printf("X_AS=%d, Y_AS=%d, Z_AS=%d\n",
  //            gyro_sensor.value(X_AS),
  //            gyro_sensor.value(Y_AS),
  //            gyro_sensor.value(Z_AS));
  //    printf("PRESS=%u, TEMP=%d\n\n",
  //            pressure_sensor.value(PRESS),
  //            pressure_sensor.value(TEMP));
  //
  //  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
