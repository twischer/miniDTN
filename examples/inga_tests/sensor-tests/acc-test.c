#include <stdio.h>
#include "contiki.h"
#include "../test.h"
#include "dev/acc-sensor.h"
#include "sys/test.h"
#include "../sensor-tests.h"

#define sqrt(a) ((a)*(a))

static char * test_acc_init();
static char * test_acc_deinit();
static char * test_acc_conf_odr();
static char * test_acc_value();
static char * assert_acc_value();
/*---------------------------------------------------------------------------*/
static char *
test_acc_init()
{

  ASSERT("activating acc failed", SENSORS_ACTIVATE(acc_sensor) == 1);
  acc_sensor.value(ACC_X);
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_acc_deinit()
{

  ASSERT("deactivating acc failed", SENSORS_DEACTIVATE(acc_sensor) == 1);
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_acc_conf_odr()
{

  ASSERT("ODR 0.10 Hz failed", acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_0HZ10) == 1);
  ASSERT("ODR 6.25 Hz failed", acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_6HZ25) == 1);
  ASSERT("ODR 25 Hz failed", acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_25HZ) == 1);
  ASSERT("ODR 100 Hz failed", acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_100HZ) == 1);
  ASSERT("ODR 400 Hz failed", acc_sensor.configure(ACC_CONF_DATA_RATE, ACC_400HZ) == 1);
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
test_acc_value()
{
  ASSERT("setting 2g failed", acc_sensor.configure(ACC_CONF_SENSITIVITY, ACC_2G) == 1);
  assert_acc_value();
  ASSERT("setting 4g failed", acc_sensor.configure(ACC_CONF_SENSITIVITY, ACC_4G) == 1);
  assert_acc_value();
  ASSERT("setting 8g failed", acc_sensor.configure(ACC_CONF_SENSITIVITY, ACC_8G) == 1);
  assert_acc_value();
  ASSERT("setting 16g failed", acc_sensor.configure(ACC_CONF_SENSITIVITY, ACC_16G) == 1);
  assert_acc_value();
  return 0;
}
/*---------------------------------------------------------------------------*/
static char *
assert_acc_value()
{
  int idx;
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;

  for (idx = 0; idx < 2; idx++) {
    x += acc_sensor.value(ACC_X);
    y += acc_sensor.value(ACC_Y);
    z += acc_sensor.value(ACC_Z);
  }
  x /= idx;
  y /= idx;
  z /= idx;
  // calc ^2 of vec length
  int32_t static_g = (sqrt(x) + sqrt(y) + sqrt(z));
  // test for 1g +/- 10%
  ASSERT("static g measurement invalid", static_g < sqrt(1000 * 1.1));
  ASSERT("static g measurement invalid", static_g > sqrt(1000 * 0.9));
  return 0;
}
/*---------------------------------------------------------------------------*/
int
run_tests()
{
  test_acc_init();
  test_acc_value();
  test_acc_deinit();
  
  return errors;
}

AUTOSTART_PROCESSES(&test_process);
