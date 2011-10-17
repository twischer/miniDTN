/* Pressure sensor interface 
 * Author  : Georg von Zengen
 * Created : 2011/10/17
 */
#include "contiki.h"
#include "lib/sensors.h"
#include "interfaces/pressure-bmp085.h"
#include "dev/pressure-sensor.h"
const struct sensors_sensor press_sensor;
uint8_t state=0;
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case PRESS:
    return (uint16_t) bmp085_read_comp_pressure(0);
  case  TEMP:
    return (uint16_t) bmp085_read_temperature();
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
  return bmp085_init();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(press_sensor, "PRESSURE", value, configure, status);
