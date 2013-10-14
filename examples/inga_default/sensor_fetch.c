#include <string.h>

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
#include "default_app.h"
#include "microSD.h"

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
static sensor_fetch_t sensor_buffer = {};
static uint16_t clock_interval = CLOCK_SECOND;

static uint8_t _configure_temppress_sensor(void);
static uint8_t _configure_gyro_sensor(void);
static uint8_t _configure_acc_sensor(void);
static uint8_t _configure_battery_sensor(void);

static void sensors_out_usb(const char* buf);
static void sensors_out_sd(uint32_t stamp, const char* buf);
static void sensors_out_radio(char* buf);
static void sensors_to_csv(sensor_fetch_t* data, char* buf);

PROCESS(sensor_update, "Sensor update");

static unsigned short max_rate_ = 0;
/*----------------------------------------------------------------------------*/
void
configure_sensors()
{
  max_rate_ = 0;
  // save process state to restore
  unsigned char sensor_update_state_ = sensor_update.state;

  printf("s_up_state: %d\n", sensor_update_state_);

  // kill sensor process if running
  if (sensor_update_state_ == 1) {
    process_exit(&sensor_update);
  }

  /* Configure battery sensor */
  if (_configure_battery_sensor() == 0) {
    sysflag_set_en(SYS_SENSOR_BATT_EN);
  } else {
    SENSORS_DEACTIVATE(battery_sensor);
    sysflag_set_dis(SYS_SENSOR_BATT_EN);
  }

  /* Configure accelerometer */
  if (_configure_acc_sensor() == 0) {
    sysflag_set_en(SYS_SENSOR_ACC_EN);
  } else {
    SENSORS_DEACTIVATE(acc_sensor);
    sysflag_set_dis(SYS_SENSOR_ACC_EN);
  }

  /* Configure gyroscoe */
  if (_configure_gyro_sensor() == 0) {
    sysflag_set_en(SYS_SENSOR_GYRO_EN);
  } else {
    SENSORS_DEACTIVATE(gyro_sensor);
    sysflag_set_dis(SYS_SENSOR_GYRO_EN);
  }

  /* Configure pressure and temperature sensor. */
  if (_configure_temppress_sensor() == 0) {
    if (system_config.temp.enabled) {
      sysflag_set_en(SYS_SENSOR_TEMP_EN);
    } else {
      sysflag_set_dis(SYS_SENSOR_TEMP_EN);
    }
    if (system_config.pressure.enabled) {
      sysflag_set_en(SYS_SENSOR_PRESS_EN);
    } else {
      sysflag_set_dis(SYS_SENSOR_PRESS_EN);
    }
  } else {
    SENSORS_DEACTIVATE(pressure_sensor);
    sysflag_set_dis(SYS_SENSOR_TEMP_EN);
    sysflag_set_dis(SYS_SENSOR_PRESS_EN);
  }

  log_v("Update rate: %d\n", max_rate_);
  /* calculate new update interval */
  clock_interval = CLOCK_SECOND * 10 / max_rate_;

  /* restore sensor process state */
  if (sensor_update_state_ == 1) {// running?
    process_start(&sensor_update, NULL);
  }
}
/*----------------------------------------------------------------------------*/
/* Configures battery sensor depending on system settings.
 * \return return 0 if succeeded, otherwise error code */
static uint8_t
_configure_battery_sensor(void)
{
  if (!system_config.battery.enabled) {
    log_i("Disabling battery sensor\n");
    SENSORS_DEACTIVATE(battery_sensor);
    return 0;
  }

  log_i("Configuring battery sensor\n");

  if (!SENSORS_ACTIVATE(battery_sensor)) {
    log_e("Failed enabling battery sensor\n");
    return 1;
  }

  max_rate_ = system_config.battery.rate > max_rate_ ? system_config.battery.rate : max_rate_;
  return 0;
}
/*----------------------------------------------------------------------------*/
/* Configures accelerometer depending on system settings.
 * \return return 0 if succeeded, otherwise error code */
static uint8_t
_configure_acc_sensor(void)
{
#if INGA_REVISION < 14
  // needed to unblock SPI at INGA V1.4
  if (system_config.acc.enabled && !system_config.output.sdcard) {
    cfs_fat_flush();
    microSD_switchoff();
    sysflag_set_dis(SYS_SD_MOUNTED);
  } else if (system_config.acc.enabled && system_config.output.sdcard) {
    log_e("SPI conflict between SD Card and accelerometer\n");
    return 128;
  }
#endif

  if (!system_config.acc.enabled) {
    log_i("Disabling accelerometer\n");
    SENSORS_DEACTIVATE(acc_sensor);
    return 0;
  }

  log_i("Configuring accelerometer\n");

  if (!SENSORS_ACTIVATE(acc_sensor)) {
    log_e("Failed enabling accelerometer\n");
    return 1;
  }
  if (!acc_sensor.configure(ACC_CONF_SENSITIVITY, system_config.acc.g_range)) {
    log_e("Failed setting accelerometer g range\n");
    return 2;
  }
  if (!acc_sensor.configure(ACC_CONF_DATA_RATE, system_config.acc.rate)) {
    log_e("Failed setting accelerometer data rate\n");
    return 3;
  }

  max_rate_ = system_config.acc.rate > max_rate_ ? system_config.acc.rate : max_rate_;
  return 0;
}
/*----------------------------------------------------------------------------*/
/* Configures gyroscope depending on system settings.
 * \return return 0 if succeeded, otherwise error code */
static uint8_t
_configure_gyro_sensor(void)
{
  if (!system_config.gyro.enabled) {
    log_i("Disabling gyroscope\n");
    SENSORS_DEACTIVATE(gyro_sensor);
    return 0;
  }

  log_i("Configuring gyroscope\n");

  if (!SENSORS_ACTIVATE(gyro_sensor)) {
    log_e("Failed enabling gyroscope\n");
    return 1;
  }
  if (!gyro_sensor.configure(GYRO_CONF_SENSITIVITY, system_config.gyro.dps)) {
    log_e("Failed setting gyro dps range\n");
    return 2;
  }
  if (!gyro_sensor.configure(GYRO_CONF_DATA_RATE, system_config.gyro.rate)) {
    log_e("Failed setting gyro data rate\n");
    return 3;
  }

  max_rate_ = system_config.gyro.rate > max_rate_ ? system_config.gyro.rate : max_rate_;
  return 0;
}
/*----------------------------------------------------------------------------*/
/* Configures temperature/pressure sensor depending on system settings.
 * \return returns 0 if succeeded, otherwise error code */
static uint8_t
_configure_temppress_sensor(void)
{
  if (!(system_config.pressure.enabled || system_config.temp.enabled)) {
    log_i("Disabling pressure/temperature sensor\n");
    SENSORS_DEACTIVATE(pressure_sensor);
    return 0;
  }

  log_i("Configuring pressure/temperature sensor\n");

  if (!SENSORS_ACTIVATE(pressure_sensor)) {
    log_e("Failed enabling pressure/temperature sensor\n");
    return 1;
  }

  max_rate_ = system_config.pressure.rate > max_rate_ ? system_config.pressure.rate : max_rate_;
  max_rate_ = system_config.temp.rate > max_rate_ ? system_config.temp.rate : max_rate_;
  return 0;
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

    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

    etimer_set(&timer, clock_interval);
    sensor_buffer.stamp = ((clock_ext << 16) + clock_time()) * 1000 / CLOCK_SECOND;

    //-- fetch all enabled sensors

    if (sysflag_get(SYS_SENSOR_BATT_EN)) {
      sensor_buffer.batt_volt = battery_sensor.value(BATTERY_VOLTAGE);
      sensor_buffer.batt_curr = battery_sensor.value(BATTERY_CURRENT);
    }

    if (sysflag_get(SYS_SENSOR_ACC_EN)) {
      sensor_buffer.acc.x = acc_sensor.value(ACC_X);
      sensor_buffer.acc.y = acc_sensor.value(ACC_Y);
      sensor_buffer.acc.z = acc_sensor.value(ACC_Z);
    }

    if (sysflag_get(SYS_SENSOR_GYRO_EN)) {
      sensor_buffer.gyro.x = gyro_sensor.value(GYRO_X);
      sensor_buffer.gyro.y = gyro_sensor.value(GYRO_Y);
      sensor_buffer.gyro.z = gyro_sensor.value(GYRO_Z);
    }

    if (sysflag_get(SYS_SENSOR_PRESS_EN)) {
      sensor_buffer.press = pressure_sensor.value(PRESS);
    }

    if (sysflag_get(SYS_SENSOR_TEMP_EN)) {
      sensor_buffer.temp = pressure_sensor.value(TEMP);
    }

    //-- Data formatting

    char buf[128];
    if ((system_config.output.usb) || (system_config.output.sdcard)) {
      printf("Converting to SD...\n");
      sensors_to_csv(&sensor_buffer, buf);
    }

    //-- Data output

    if (system_config.output.usb) {
      sensors_out_usb(buf);
    }

    if ((system_config.output.sdcard) && (sysflag_get(SYS_SD_MOUNTED))) {
      sensors_out_sd(sensor_buffer.stamp, buf);
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
sensors_out_usb(const char* data)
{
  printf(data);
}
/*----------------------------------------------------------------------------*/
static void
sensors_out_sd(uint32_t stamp, const char* data)
{
  int fd;
  static uint8_t open_ = 0;
  // determine file name
  char fname_buf[9];
  sprintf(fname_buf, "o%ld.dat", stamp / 1000); // every 10 seconds
  //  if (!open_) {
  // And open it
  fd = cfs_open(fname_buf, CFS_APPEND);

  // In case something goes wrong, we cannot save this file
  if (fd == -1) {
    log_e("Failed opening output file\n");
    return -1;
  }

  //    open_ = 1;
  //  }
  printf("writing %d bytes...", strlen(data));

  int size = cfs_write(fd, data, strlen(data));
  printf("done.\n");
  printf("flushing...");
  cfs_fat_flush();
  printf("done.\n");

  printf("closing...");
  cfs_close(fd);
  printf("done.\n");

}
/*----------------------------------------------------------------------------*/
void
sensors_out_radio(char* buf)
{

}
