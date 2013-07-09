#include <stdio.h>
#include "sys/test.h"
#include "dev/gyro-sensor.h"
#include "../test.h"
#include "../sensor-tests.h"

static char * test_gyro_init();
/*---------------------------------------------------------------------------*/
static char *
test_gyro_init()
{

  ASSERT("activating acc failed", SENSORS_ACTIVATE(gyro_sensor) == 1);
  return 0;
}
/*---------------------------------------------------------------------------*/
int
run_tests()
{
  test_gyro_init();
  return errors;
}

AUTOSTART_PROCESSES(&test_process);
