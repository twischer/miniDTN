#include <stdio.h>
#include <util/delay.h>
#include "contiki.h"
#include "../test.h"
#include "dev/acc-sensor.h"
#include "sensor-tests.h"

/*--- Test parameters ---*/
#define ACC_TEST_CFG_MIN_ACC    800L
#define ACC_TEST_CFG_MAX_ACC    1200L
/*--- ---*/

struct sensors_sensor *test_sensor; // TODO: use

TEST_SUITE("acc_test");

/*---------------------------------------------------------------------------*/
void
test_acc_find()
{
  test_sensor_find(ACC_SENSOR, test_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_acc_init()
{
  test_sensor_activate(acc_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_acc_deinit()
{
  test_sensor_deactivate(acc_sensor);
}
/*---------------------------------------------------------------------------*/
void
test_acc_conf_odr()
{
  TEST_PRE();
  TEST_EQUALS(acc_sensor.status(SENSORS_ACTIVE), 1);
  
  TEST_CODE();
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_0HZ10), 1);
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_6HZ25), 1);
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_25HZ), 1);
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_100HZ), 1);
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_400HZ), 1);
}
/*---------------------------------------------------------------------------*/
void
assert_acc_value()
{
  int idx;
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;

  // wait for stable values
  _delay_ms(100);

  for (idx = 0; idx < 2; idx++) {
    x += acc_sensor.value(ACC_X);
    y += acc_sensor.value(ACC_Y);
    z += acc_sensor.value(ACC_Z);
  }
  x >>= 1;
  y >>= 1;
  z >>= 1;
  // calc ^2 of vec length
  int32_t static_g = x*x + y*y + z*z;
  // test for 1g +/- 10%
  TEST_REPORT("x-axis", (int16_t) x, 1, "mg");
  TEST_REPORT("y-axis", (int16_t) y, 1, "mg");
  TEST_REPORT("z-axis", (int16_t) z, 1, "mg");

  TEST_LEQ(static_g, ACC_TEST_CFG_MAX_ACC*ACC_TEST_CFG_MAX_ACC);
  TEST_GEQ(static_g, ACC_TEST_CFG_MIN_ACC*ACC_TEST_CFG_MIN_ACC);
}
/*---------------------------------------------------------------------------*/
void
test_acc_value()
{
  TEST_PRE();
  TEST_EQUALS(acc_sensor.status(SENSORS_ACTIVE), 1);
  
  TEST_CODE();
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_SENSITIVITY, ACC_2G), 1);
  assert_acc_value();
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_SENSITIVITY, ACC_4G), 1);
  assert_acc_value();
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_SENSITIVITY, ACC_8G), 1);
  assert_acc_value();
  TEST_EQUALS(acc_sensor.configure(ACC_CONF_SENSITIVITY, ACC_16G), 1);
  assert_acc_value();
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
  
  RUN_TEST("test_acc_find", test_acc_find);
  RUN_TEST("test_acc_init", test_acc_init);
  RUN_TEST("acc_conf_odr", test_acc_conf_odr);
  RUN_TEST("acc_value", test_acc_value);
  RUN_TEST("acc_deact", test_acc_deinit);
  
  TESTS_DONE();

  PROCESS_END();
}

AUTOSTART_PROCESSES(&test_process);
