/* 
 * File:   sensor_tests.h
 * Author: enrico
 *
 * Created on 13. März 2013, 01:21
 */

#ifndef SENSOR_TESTS_H
#define	SENSOR_TESTS_H

#include <sensors.h>

//int run_tests();

//PROCESS_NAME(test_process);
void test_sensor_find(char* name, struct sensors_sensor *psensor);
void test_sensor_activate(struct sensors_sensor sensor);
/*---------------------------------------------------------------------------*/
void test_sensor_deactivate(struct sensors_sensor sensor);
#endif	/* SENSOR_TESTS_H */

