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

PROCESS(test_process, "Test process");
/*---------------------------------------------------------------------------*/
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
}
