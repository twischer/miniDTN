#include <stdio.h>
#include "test.h"
#include "dev/gyro-sensor.h"

static char * test_gyro_init();
/*---------------------------------------------------------------------------*/
static char *
test_gyro_init()
{
  printf("test_init...\n");
  ASSERT("activating acc failed", SENSORS_ACTIVATE(gyro_sensor) == 1);
  return 0;
}
/*---------------------------------------------------------------------------*/
char *
gyro_tests()
{
  RUN_TEST(test_gyro_init);
  return 0;
}
