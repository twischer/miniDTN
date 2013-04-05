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
#include "battery-sensor.h"
#include "cfs.h"

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
} vec_int16_t;

/** This struct is used to store all collected sensor data of a single readout. */
typedef struct {
  uint32_t stamp;
  uint16_t batt_volt;
  uint16_t batt_curr;
  vec_int16_t acc;
  vec_int16_t gyro;
  uint16_t press;
  uint16_t temp;
} sensor_fetch_t;

static struct etimer timer;
static sensor_fetch_t sensor_fetch = {};
static uint16_t clock_interval = CLOCK_SECOND;
static void sensors_out_usb(char* buf);
static void sensors_out_sd(char* buf);
static void sensors_out_radio(char* buf);
static void sensors_to_csv(sensor_fetch_t* data, char* buf);
PROCESS(sensor_update, "Sensor update");
/*----------------------------------------------------------------------------*/
void
init_sensors()
{
  unsigned short max_rate_ = 0;
  // sensor setup

  // kill sensor process
  process_exit(&sensor_update);

  /* Initialize battery sensor if enabled */
  if (system_config.battery.enabled) {
    log_i("Configuring battery sensor\n");
    if (!SENSORS_ACTIVATE(battery_sensor)) {
      log_e("Failed enabling battery sensor\n");
    }
    max_rate_ = system_config.battery.rate > max_rate_ ? system_config.battery.rate : max_rate_;
  } else {
    SENSORS_DEACTIVATE(battery_sensor);
  }

  /* Initialize accelerometer if enabled */
  if (system_config.acc.enabled) {
    log_i("Configuring accelerometer\n");
    if (SENSORS_ACTIVATE(acc_sensor)) {
      if (!acc_sensor.configure(ACC_CONF_SENSITIVITY, system_config.acc.g_range)) {
        log_e("Failed setting accelerometer g range\n");
      }
      if (!acc_sensor.configure(ACC_CONF_DATA_RATE, system_config.acc.rate)) {
        log_e("Failed setting accelerometer data rate\n");
      }
      max_rate_ = system_config.acc.rate > max_rate_ ? system_config.acc.rate : max_rate_;
    } else {
      log_e("Failed enabling accelerometer\n");
    }
  } else {
    SENSORS_DEACTIVATE(acc_sensor);
  }

  /* Initialize gyroscoe if enabled */
  if (system_config.gyro.enabled) {
    log_i("Configuring gyroscope\n");
    if (SENSORS_ACTIVATE(gyro_sensor)) {
      if (!gyro_sensor.configure(GYRO_CONF_SENSITIVITY, system_config.gyro.dps)) {
        log_e("Failed setting gyro dps range\n");
      }
      if (!gyro_sensor.configure(GYRO_CONF_DATA_RATE, system_config.gyro.rate)) {
        log_e("Failed setting gyro data rate\n");
      }
      max_rate_ = system_config.gyro.rate > max_rate_ ? system_config.gyro.rate : max_rate_;
    } else {
      log_e("Failed enabling gyroscope\n");
    }
  } else {
    SENSORS_DEACTIVATE(gyro_sensor);
  }

  /* Initialize pressure and temperature sensor if enabled */
  if (system_config.pressure.enabled || system_config.temp.enabled) {
    log_i("Configuring pressure/temperature sensor\n");
    if (SENSORS_ACTIVATE(pressure_sensor)) {
      max_rate_ = system_config.pressure.rate > max_rate_ ? system_config.pressure.rate : max_rate_;
      max_rate_ = system_config.temp.rate > max_rate_ ? system_config.temp.rate : max_rate_;
    } else {
      log_e("Failed enabling pressure/temperature sensor\n");
    }
  } else {
    SENSORS_DEACTIVATE(pressure_sensor);
  }

  log_v("Update rate: %d\n", max_rate_);
  /* calculate new update interval */
  clock_interval = CLOCK_SECOND * 10 / max_rate_;
  etimer_set(&timer, clock_interval);
  /* restart sensor process */
  process_start(&sensor_update, NULL);
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
    sensor_fetch.stamp = ((clock_ext << 16) + clock_time()) * 1000 / CLOCK_SECOND;

    //-- fetch all enabled sensors

    if (system_config.battery.enabled) {
      sensor_fetch.batt_volt = battery_sensor.value(BATTERY_VOLTAGE);
      sensor_fetch.batt_curr = battery_sensor.value(BATTERY_CURRENT);
    }

    if (system_config.acc.enabled) {
      sensor_fetch.acc.x = acc_sensor.value(ACC_X);
      sensor_fetch.acc.y = acc_sensor.value(ACC_Y);
      sensor_fetch.acc.z = acc_sensor.value(ACC_Z);
    }

    if (system_config.gyro.enabled) {
      sensor_fetch.gyro.x = gyro_sensor.value(GYRO_X);
      sensor_fetch.gyro.y = gyro_sensor.value(GYRO_Y);
      sensor_fetch.gyro.z = gyro_sensor.value(GYRO_Z);
    }

    if (system_config.pressure.enabled) {
      sensor_fetch.press = pressure_sensor.value(PRESS);
    }

    if (system_config.temp.enabled) {
      sensor_fetch.temp = pressure_sensor.value(TEMP);
    }
    
    //-- Data formatting

    char buf[128];
    if ((system_config.output.usb) || (system_config.output.sdcard)) {
      sensors_to_csv(&sensor_fetch, buf);
    }
    
    //-- Data output

    if (system_config.output.usb) {
      sensors_out_usb(buf);
    }
    
    if (system_config.output.sdcard) {
      sensors_out_sd(buf);
    }
    
    if (system_config.output.radio) {
      sensors_out_radio(buf);
    }

  }

  PROCESS_END();
}
/*----------------------------------------------------------------------------*/
static void
sensors_to_csv(sensor_fetch_t* data, char* buf)
{
  uint8_t buf_offset = 0;

  buf_offset += sprintf(buf, "%ld, ", data->stamp);

  if (system_config.battery.enabled) {
    buf_offset += sprintf(buf + buf_offset, "%d, %d, ",
            data->batt_volt,
            data->batt_curr);
  }

  if (system_config.acc.enabled) {
    buf_offset += sprintf(buf + buf_offset, "%d, %d, %d, ",
            data->acc.x,
            data->acc.y,
            data->acc.z);
  }

  if (system_config.gyro.enabled) {
    buf_offset += sprintf(buf + buf_offset, "%d, %d, %d, ",
            data->gyro.x,
            data->gyro.y,
            data->gyro.z);
  }

  if (system_config.pressure.enabled) {
    buf_offset += sprintf(buf + buf_offset, "%u, ", data->press);
  }

  if (system_config.temp.enabled) {
    buf_offset += sprintf(buf + buf_offset, "%d, ", data->temp);
  }

  sprintf(buf + buf_offset, "\n");
}
/*----------------------------------------------------------------------------*/
static void
sensors_out_usb(char* buf)
{
  printf(buf);
}
/*----------------------------------------------------------------------------*/
void
sensors_out_sd(char* data)
{
  int fd;
  static uint8_t open_ = 0;
//  if (!open_) {
    // And open it
    fd = cfs_open("out.dat", CFS_APPEND);

    // In case something goes wrong, we cannot save this file
    if (fd == -1) {
      log_e("Failed opening output file\n");
      return -1;
    }

//    open_ = 1;
//  }

  int size = cfs_write(fd, data, MAX_FILE_SIZE);
  cfs_fat_flush();
  
  cfs_close(fd);

}
/*----------------------------------------------------------------------------*/
void
sensors_out_radio(char* buf)
{

}
