#include <stdio.h>
#include "contiki.h"
#include "../test.h"
#include "sys/test.h"
#include "dev/battery-sensor.h"
#include "../sensor-tests.h"

static char * test_battery_init();
static char * test_battery_value();
/*---------------------------------------------------------------------------*/
static char *
test_battery_init()
{
  printf("test_battery_init...\n");
  ASSERT("activating battery sensor failed", SENSORS_ACTIVATE(battery_sensor) == 1);
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_battery_value()
{
  printf("test_battery_value...\n");
  // get voltage and current
  int v = battery_sensor.value(2);// TODO: BATTERY_VOLTAGE
  int i = battery_sensor.value(3);
  ASSERT("v wrong", v < 4500);
  ASSERT("v wrong", v > 3000);
  ASSERT("i wrong", i < 100);
  ASSERT("i wrong", i > 30);
  return 0;
}
/*---------------------------------------------------------------------------*/
int
run_tests()
{
  test_battery_init();
  test_battery_value();
  return errors;
}

AUTOSTART_PROCESSES(&test_process);
