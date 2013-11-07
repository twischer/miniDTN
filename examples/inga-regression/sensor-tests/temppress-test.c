#include <stdio.h>
#include "contiki.h"
#include "../test.h"
#include "sys/test.h"
#include "dev/pressure-sensor.h"
#include "sensor-tests.h"

/*--- Test parameters ---*/
#define SENSOR_NAME                   "Press"
#define TEMPPRESS_TEST_CFG_MAX_TEMP   350
#define TEMPPRESS_TEST_CFG_MIN_TEMP   200
#define TEMPPRESS_TEST_CFG_MAX_PRESS  105000L
#define TEMPPRESS_TEST_CFG_MIN_PRESS  95000L
/*--- ---*/

struct sensors_sensor *test_sensor; // TODO: use

TEST_SUITE("temppress_sensor");

void
test_temppress_find()
{
  test_sensor_find(SENSOR_NAME, test_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_temppress_init()
{
  test_sensor_activate(pressure_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_temppress_deinit()
{
  test_sensor_deactivate(pressure_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_temperature_value()
{
  int tempval = pressure_sensor.value(TEMP);
  printf("temp: %d\n", tempval);
  TEST_REPORT("Temperature", tempval, 10, "deg C");
  TEST_LEQ(tempval, TEMPPRESS_TEST_CFG_MAX_TEMP)
  TEST_GEQ(tempval, TEMPPRESS_TEST_CFG_MIN_TEMP)
}
/*---------------------------------------------------------------------------*/
void
test_pressure_value()
{
  uint16_t press_h = pressure_sensor.value(PRESS_H);
  uint16_t press_l = pressure_sensor.value(PRESS_L);
  int32_t pressval = ((int32_t) press_h << 16);
  pressval |= (pressure_sensor.value(PRESS_L) & 0xFFFF);
  TEST_REPORT("pressure", pressval, 100, "hPa");
  TEST_LEQ(pressval, TEMPPRESS_TEST_CFG_MAX_PRESS);
  TEST_GEQ(pressval, TEMPPRESS_TEST_CFG_MIN_PRESS);
}
/*---------------------------------------------------------------------------*/
void
test_pressure_mode()
{
  pressure_sensor.configure(PRESSURE_CONF_OPERATION_MODE, PRESSURE_MODE_ULTRA_LOW_POWER);
  test_pressure_value();
  pressure_sensor.configure(PRESSURE_CONF_OPERATION_MODE, PRESSURE_MODE_STANDARD);
  test_pressure_value();
  pressure_sensor.configure(PRESSURE_CONF_OPERATION_MODE, PRESSURE_MODE_HIGH_RES);
  test_pressure_value();
  pressure_sensor.configure(PRESSURE_CONF_OPERATION_MODE, PRESSURE_MODE_ULTRA_HIGH_RES);
  test_pressure_value();
}
/*---------------------------------------------------------------------------*/

static struct etimer timer;

PROCESS(test_process, "Test process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();
  
  // Wait a second...
  etimer_set(&timer, CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
  
  RUN_TEST("temppress_find", test_temppress_find);
  RUN_TEST("temppress_init", test_temppress_init);
  RUN_TEST("temperature_value", test_temperature_value);
  RUN_TEST("pressure_mode", test_pressure_mode);
  RUN_TEST("temppress_deinit", test_temppress_deinit);
  
  TESTS_DONE();

  PROCESS_END();
}

AUTOSTART_PROCESSES(&test_process);
