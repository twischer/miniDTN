/* 
 * File:   sensor-tests.c
 * Author: enrico
 *
 * Created on 9. Juli 2013, 12:21
 */
#include <stdio.h>
#include "contiki.h"
#include "../test.h"
#include "sys/test.h"
#include "sensor-tests.h"
/*
 * 
 */
static struct etimer timer;

//PROCESS(test_process, "Test process");
/*---------------------------------------------------------------------------*/
/*
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();
  
  // Wait a second...
  etimer_set(&timer, CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));
  
  errors = run_tests();
  
  if (errors == 0) {
    TEST_PASS();
  } else {
    TEST_FAIL("There were errors");
  }
  
  PROCESS_END();
}*/

void
test_sensor_find(char* name, struct sensors_sensor *psensor)
{
  TEST_CODE();
  psensor = sensors_find(name);
  TEST_NEQ(psensor, NULL);
}
/*---------------------------------------------------------------------------*/
void
test_sensor_activate(struct sensors_sensor sensor)
{
//  TEST_BEGIN(name);

  TEST_PRE();
  TEST_EQUALS(sensor.status(SENSORS_READY), 1);
  TEST_EQUALS(sensor.status(SENSORS_ACTIVE), 0);
  
  TEST_CODE();
  TEST_EQUALS(SENSORS_ACTIVATE(sensor), 1);
  
  TEST_POST();
  TEST_EQUALS(sensor.status(SENSORS_ACTIVE), 1);
  
//  TEST_END();
}
/*---------------------------------------------------------------------------*/
void
test_sensor_deactivate(struct sensors_sensor sensor)
{
//  TEST_BEGIN(name);

  TEST_CODE();
  TEST_EQUALS(SENSORS_DEACTIVATE(sensor), 1);

  TEST_POST();
  //TEST_EQUALS(sensor.status(SENSORS_ACTIVE), 0);  @TODO: check
  TEST_EQUALS(sensor.status(SENSORS_READY), 1);

//  TEST_END();
}
