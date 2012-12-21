/* Acceleration sensor interface 
 * Author  : Georg von Zengen
 * Created : 2011/10/17
 */
#include "contiki.h"
#include "lib/sensors.h"
#include "adxl345.h"
#include "adxl345-sensor.h"
const struct sensors_sensor acc_sensor;
uint8_t acc_state=0;
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case ADXL345_X:
    return adxl345_get_x_acceleration();

  case ADXL345_Y:
    return adxl345_get_y_acceleration();

  case ADXL345_Z:
    return adxl345_get_z_acceleration();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch (type) {
    case SENSORS_ACTIVE:
      break;
    case SENSORS_READY:
      break;
  }
  return acc_state;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch (type) {
    case SENSORS_ACTIVE:
      if (c) {
        if (!status(SENSORS_ACTIVE)) {
          // TODO...
        }
        acc_state=1;// TODO: SENSORS_READY?
        return adxl345_init();
      } else {
        // deactivate
      }
      break;
    case ADXL345_SENSOR_SENSITIVITY:
      // TODO: check if initialized?
      adxl345_set_g_range(c);
      break;
    case ADXL345_SENSOR_FIFOMODE:
      adxl345_set_fifomode(c);
      break;
    case ADXL345_SENSOR_POWERMODE:
      adxl345_set_powermode(c);
      break;
    default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(acc_sensor, "ACCELERATION", value, configure, status);
