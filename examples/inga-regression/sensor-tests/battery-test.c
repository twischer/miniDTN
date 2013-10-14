#include <stdio.h>
#include "contiki.h"
#include "../test.h"
#include "sys/test.h"
#include "dev/battery-sensor.h"
#include "sensor-tests.h"

/*--- Test parameters ---*/
#define SENSOR_NAME             "BATTERY"
#define BATTERY_TEST_CFG_MIN_V  3000
#define BATTERY_TEST_CFG_MAX_V  4500
#define BATTERY_TEST_CFG_MIN_I  30
#define BATTERY_TEST_CFG_MAX_I  100
/*--- ---*/

struct sensors_sensor *test_sensor; // TODO: use

TEST_SUITE("temppress_sensor");

/*---------------------------------------------------------------------------*/
void
test_battery_init()
{
  test_sensor_activate(battery_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_battery_deinit()
{
  test_sensor_deactivate(battery_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_battery_value()
{
  TEST_PRE();
  
  
  TEST_CODE();
  printf("test_battery_value...\n");
  // get voltage and current
  int v = battery_sensor.value(BATTERY_VOLTAGE);
  int i = battery_sensor.value(BATTERY_CURRENT);
  TEST_LEQ(v, BATTERY_TEST_CFG_MAX_V);
  TEST_GEQ(v, BATTERY_TEST_CFG_MIN_V);
  TEST_LEQ(i, BATTERY_TEST_CFG_MAX_I);
  TEST_GEQ(i, BATTERY_TEST_CFG_MIN_I);
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
  
  RUN_TEST("battery_init", test_battery_init);
  RUN_TEST("battery_value", test_battery_value);
  RUN_TEST("battery_deinit", test_battery_deinit);
  
  TESTS_DONE();
  
  PROCESS_END();
}

AUTOSTART_PROCESSES(&test_process);
