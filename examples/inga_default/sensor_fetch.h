/* 
 * File:   sensor_fetch.h
 * Author: enrico
 *
 * Created on 7. Februar 2013, 03:36
 */

#ifndef SENSOR_FETCH_H
#define	SENSOR_FETCH_H

/**
 * @addtogroup default_app
 * @{
 */

/**
 * \defgroup sensor_fetch Sensor data fetch
 * @{
 */

#include "app_config.h"

/**
 * Configures all available sensors depending on system_config settings
 * 
 * Stops and restarts sensor_update process if it was running.
 */
void configure_sensors();

/**
 * Sensor update process.
 * 
 * Aggregates data off all available sensors.
 * After start it is triggered by timer set by configure_sensors().
 */
PROCESS_NAME(sensor_update);

/** @} */
/** @} */

#endif	/* SENSOR_FETCH_H */

