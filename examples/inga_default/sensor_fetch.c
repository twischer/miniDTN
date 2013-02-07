#include "sensor_fetch.h"
#include "logger.h"
#include "contiki.h"
#include "app_config.h"
#include "sensors.h"
#include "acc-sensor.h"
#include "adc-sensor.h"
#include "button-sensor.h"
#include "gyro-sensor.h"
#include "pressure-sensor.h"
#include "radio-sensor.h"

static struct etimer timer;
static uint16_t clock_interval = CLOCK_SECOND;
PROCESS(sensor_update, "Sensor update");
/*----------------------------------------------------------------------------*/
void
init_sensors()
{
  unsigned short max_rate_ = 0;
  // sensor setup
  log_i("Initial rate is: %u\n", max_rate_);
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
    max_rate_ = system_config.acc.rate > max_rate_ ? system_config.acc.rate : max_rate_;
  }
  log_i("Acc rate is: %u\n", system_config.acc.rate);
  if (system_config.gyro.enabled) {
    log_i("Enabling gyroscope\n");
    SENSORS_ACTIVATE(gyro_sensor);
    max_rate_ = system_config.gyro.rate > max_rate_ ? system_config.gyro.rate : max_rate_;
  }
  log_i("Gyro rate is: %u\n", system_config.gyro.rate);
  if (system_config.pressure.enabled || system_config.temp.enabled) {
    log_i("Enabling pressure/temperature sensor\n");
    SENSORS_ACTIVATE(pressure_sensor);
    max_rate_ = system_config.pressure.rate > max_rate_ ? system_config.pressure.rate : max_rate_;
    max_rate_ = system_config.temp.rate > max_rate_ ? system_config.temp.rate : max_rate_;
  }
  log_i("Temps rate is: %u\n", system_config.temp.rate);
  log_i("Pressure rate is: %u\n", system_config.pressure.rate);

  log_i("Maximum rate is: %u\n", max_rate_);
  // calculate new interval
  //  clock_interval = ((uint32_t) CLOCK_SECOND * max_rate_) / 1000UL;
}
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_update, ev, data)
{
  PROCESS_BEGIN();

  etimer_set(&timer, clock_interval);

  while (1) {

    PROCESS_YIELD();
    etimer_set(&timer, clock_interval);
    printf("X_ACC=%d, Y_ACC=%d, Z_ACC=%d\n",
            acc_sensor.value(ACC_X),
            acc_sensor.value(ACC_Y),
            acc_sensor.value(ACC_Z));
    printf("X_AS=%d, Y_AS=%d, Z_AS=%d\n",
            gyro_sensor.value(X_AS),
            gyro_sensor.value(Y_AS),
            gyro_sensor.value(Z_AS));
    printf("PRESS=%u, TEMP=%d\n\n",
            pressure_sensor.value(PRESS),
            pressure_sensor.value(TEMP));

  }

  PROCESS_END();
}
