//#include <stdio.h>
#include <stdlib.h>
#include "sys/test.h"
#include "util/delay.h"
#include "dev/gyro-sensor.h"
#include "../test.h"
#include "sensor-tests.h"

/*--- Test parameters ---*/
#define SENSOR_NAME GYRO_SENSOR 
#define GYRO_TEST_CFG_MAX_ZERO_G      1
#define GYRO_TEST_CFG_MAX_ZERO_G_RAW  150
/*--- ---*/

struct sensors_sensor *test_sensor; // TODO: use

TEST_SUITE("gyro_test");

/*---------------------------------------------------------------------------*/
void
test_gyro_find()
{
  test_sensor_find(SENSOR_NAME, test_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_gyro_init()
{
  test_sensor_activate(gyro_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_gyro_deinit()
{
  test_sensor_deactivate(gyro_sensor);
}
/*---------------------------------------------------------------------------*/
void
channel_test() {
   // wait for stable values
  _delay_ms(500);

  printf("x-axis: %d\n", gyro_sensor.value(GYRO_X));
  printf("y-axis: %d\n", gyro_sensor.value(GYRO_Y));
  printf("z-axis: %d\n", gyro_sensor.value(GYRO_Z));

  TEST_LEQ(abs(gyro_sensor.value(GYRO_X_RAW)), GYRO_TEST_CFG_MAX_ZERO_G_RAW);
  TEST_LEQ(abs(gyro_sensor.value(GYRO_Y_RAW)), GYRO_TEST_CFG_MAX_ZERO_G_RAW);
  TEST_LEQ(abs(gyro_sensor.value(GYRO_X_RAW)), GYRO_TEST_CFG_MAX_ZERO_G_RAW);

  TEST_LEQ(abs(gyro_sensor.value(GYRO_X)), GYRO_TEST_CFG_MAX_ZERO_G);
  TEST_LEQ(abs(gyro_sensor.value(GYRO_Y)), GYRO_TEST_CFG_MAX_ZERO_G);
  TEST_LEQ(abs(gyro_sensor.value(GYRO_Z)), GYRO_TEST_CFG_MAX_ZERO_G);
}
/*---------------------------------------------------------------------------*/
void
test_gyro_value()
{

  TEST_EQUALS(gyro_sensor.configure(GYRO_CONF_SENSITIVITY, GYRO_250DPS), 1);
  channel_test();
  TEST_EQUALS(gyro_sensor.configure(GYRO_CONF_SENSITIVITY, GYRO_500DPS), 1);
  channel_test();
  TEST_EQUALS(gyro_sensor.configure(GYRO_CONF_SENSITIVITY, GYRO_2000DPS), 1);
  channel_test();
}
/*---------------------------------------------------------------------------*/
void
test_gyro_dps_settings()
{
  TEST_EQUALS(gyro_sensor.configure(GYRO_CONF_SENSITIVITY, GYRO_250DPS), 1);
  TEST_EQUALS(gyro_sensor.configure(GYRO_CONF_SENSITIVITY, GYRO_500DPS), 1);
  TEST_EQUALS(gyro_sensor.configure(GYRO_CONF_SENSITIVITY, GYRO_2000DPS), 1);
}

static struct etimer timer;

PROCESS(test_process, "Test process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();
  
  // Wait a second...
  etimer_set(&timer, CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));

  RUN_TEST("gyro_find", test_gyro_find);
  RUN_TEST("gyro_init", test_gyro_init);
  RUN_TEST("gyro_value", test_gyro_value);
  RUN_TEST("gyro_dps_settings", test_gyro_dps_settings);
  RUN_TEST("gyro_deinit", test_gyro_deinit);
  
  TESTS_DONE();

  PROCESS_END();
}

AUTOSTART_PROCESSES(&test_process);
