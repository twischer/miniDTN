/* Pressure sensor interface 
 * Author  : Georg von Zengen
 * Created : 2011/10/17
 */
#include "contiki.h"
#include "lib/sensors.h"
#include "interfaces/pressure-bmp085.h"
#include "dev/pressure-sensor.h"
const struct sensors_sensor pressure_sensor;
uint8_t press_state=0;
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case PRESS:
    return (uint16_t) bmp085_read_comp_pressure(0);
  case  TEMP:
    //return (uint16_t) bmp085_read_temperature();
    return (int16_t) bmp085_read_comp_temperature();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  return press_state;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  press_state=1;
  return bmp085_init();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(pressure_sensor, "PRESSURE", value, configure, status);
