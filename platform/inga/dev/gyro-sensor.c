/** GYROSCOPE sensor interface 
 * @author  : Georg von Zengen
 * @date : 2011/10/17
 * @todo: License here?
 */
#include "contiki.h"
#include "lib/sensors.h"
#include "l3g4200d.h"
#include "gyro-sensor.h"
const struct sensors_sensor gyro_sensor;
uint8_t gyro_state=0;
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case X_AS:
    return l3g4200d_get_x_angle();

  case Y_AS:
    return l3g4200d_get_z_angle();

  case Z_AS:
    return l3g4200d_get_y_angle();

  case TEMP_AS:
		return (uint8_t) l3g4200d_get_temp();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  return gyro_state;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  gyro_state=1;
  return l3g4200d_init();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(gyro_sensor, "GYROSCOPE", value, configure, status);
