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

void init_sensors();

PROCESS_NAME(sensor_update);

/** @} */
/** @} */

#endif	/* SENSOR_FETCH_H */

