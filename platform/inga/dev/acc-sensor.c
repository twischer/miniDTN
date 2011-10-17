/* Acceleration sensor interface 
 * Author  : Georg von Zengen
 * Created : 2011/10/17
 */
#include "contiki.h"
#include "lib/sensors.h"
#include "interfaces/acc-adxl345.h"
#include "dev/acc-sensor.h"
const struct sensors_sensor light_sensor;
uint8_t state=0;
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case X_ACC:
    return adxl345_get_x_acceleration();

  case Y_ACC:
    return adxl345_get_y_acceleration();

  case Z_ACC:
    return adxl345_get_z_acceleration();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  return state;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  state=1;
  return adxl345_init();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(acc_sensor, "ACCELERATION", value, configure, status);
