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

#include "dev/adxl345.h"             //tested
#include "dev/microSD.h"           //tested
#include "dev/at45db.h"            //tested
#include "dev/bmp085.h"         //tested
#include "dev/l3g4200d.h"           //tested
#include "dev/mpl115a.h"        //tested (not used)
#include "dev/adc.h"

static struct etimer timer;
static uint16_t clock_interval = CLOCK_SECOND;
PROCESS(sensor_update, "Sensor update");
/*----------------------------------------------------------------------------*/
void
init_sensors()
{
  unsigned short max_rate_ = 0;
  // sensor setup

  /* Initialize accelerometer if enabled */
  if (system_config.acc.enabled) {
    log_i("Configuring accelerometer\n");
    if (!SENSORS_ACTIVATE(acc_sensor)) {
      log_e("Failed enabling accelerometer\n");
      SENSORS_ACTIVATE(acc_sensor);
      //      acc_sensor.configure(ACC_SENSOR_FIFOMODE, ACC_MODE_STREAM);
    }
    if (!acc_sensor.configure(ACC_CONF_SENSITIVITY, system_config.acc.g_range)) {
      log_e("Failed setting accelerometer g range\n");
    }
    if (!acc_sensor.configure(ACC_CONF_DATA_RATE, system_config.acc.rate)) {
      log_e("Failed setting accelerometer data rate\n");
    }
    max_rate_ = system_config.acc.rate > max_rate_ ? system_config.acc.rate : max_rate_;
  }

  /* Initialize gyroscoe if enabled */
  if (system_config.gyro.enabled) {
    log_i("Configuring gyroscope\n");
    if (!SENSORS_ACTIVATE(gyro_sensor)) {
      log_e("Failed enabling gyroscope\n");
    }
    if(!gyro_sensor.configure(GYRO_CONF_SENSITIVITY, system_config.gyro.dps)) {
      log_e("Failed setting gyro dps range\n");
    }
    if(!gyro_sensor.configure(GYRO_CONF_DATA_RATE, system_config.gyro.rate)) {
      log_e("Failed setting gyro data rate\n");
    }
    max_rate_ = system_config.gyro.rate > max_rate_ ? system_config.gyro.rate : max_rate_;
  }

  /* Initialize pressure and temperature sensor if enabled */
  if (system_config.pressure.enabled || system_config.temp.enabled) {
    log_i("Configuring pressure/temperature sensor\n");
    if (!SENSORS_ACTIVATE(pressure_sensor)) {
      log_e("Failed enabling pressure/temperature sensor\n");
    }
    max_rate_ = system_config.pressure.rate > max_rate_ ? system_config.pressure.rate : max_rate_;
    max_rate_ = system_config.temp.rate > max_rate_ ? system_config.temp.rate : max_rate_;
  }

  /* calculate new update interval */
  clock_interval = CLOCK_SECOND * 10 / max_rate_;
  etimer_set(&timer, clock_interval);
}
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_update, ev, data)
{
  PROCESS_BEGIN();
  static clock_time_t last_time;
  last_time = clock_time();
  static uint16_t clock_ext = 0;

  etimer_set(&timer, clock_interval);

  while (1) {

    if (clock_time() < last_time) {
      clock_ext++;
    }

    PROCESS_YIELD();
    etimer_set(&timer, clock_interval);
    static const char buf[128];
    uint8_t buf_offset = 0;
    buf_offset += sprintf(buf, "%ld, ", ((clock_ext << 16) + clock_time()) * 1000 / CLOCK_SECOND);
    if (system_config.acc.enabled) {
      buf_offset += sprintf(buf + buf_offset, "%d, %d, %d, ",
              acc_sensor.value(ACC_X),
              acc_sensor.value(ACC_Y),
              acc_sensor.value(ACC_Z));
    }
    if (system_config.gyro.enabled) {
      buf_offset += sprintf(buf + buf_offset, "%d, %d, %d, ",
              gyro_sensor.value(GYRO_X),
              gyro_sensor.value(GYRO_Y),
              gyro_sensor.value(GYRO_Z));
    }
    if (system_config.pressure.enabled) {
      buf_offset += sprintf(buf + buf_offset, "%u, ", pressure_sensor.value(PRESS));
    }
    if (system_config.temp.enabled) {
      buf_offset += sprintf(buf + buf_offset, "%d, ", pressure_sensor.value(TEMP));
    }
    sprintf(buf + buf_offset, "\n");
    printf(buf);
    //    if (system_config.adc.enabled) {
    //    }

  }

  PROCESS_END();
}
