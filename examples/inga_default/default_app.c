/**
 * default app.c
 */
#include "contiki.h"

#include <util/delay.h>
#include <stdlib.h>

#include <avr/io.h>

#define DEBUG                                   1

#include <stdio.h> /* For printf() */
#include <stdint.h>

#include "acc-sensor.h"
#include "adc-sensor.h"
#include "button-sensor.h"
#include "gyro-sensor.h"
#include "pressure-sensor.h"
#include "radio-sensor.h"

#include "ini_parser.h"
#include "app_config.h"

/*---------------------------------------------------------------------------*/
PROCESS(default_app_process, "Sensor update process");
AUTOSTART_PROCESSES(&default_app_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(default_app_process, ev, data) {
  PROCESS_BEGIN();

  app_config_load();

  print_config();

  SENSORS_ACTIVATE(acc_sensor);
  SENSORS_ACTIVATE(gyro_sensor);
  SENSORS_ACTIVATE(pressure_sensor);

  etimer_set(&timer, CLOCK_SECOND * 0.05);

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
