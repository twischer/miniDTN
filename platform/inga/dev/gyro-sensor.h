/*
 * Copyright (c) 2012, TU Braunschweig.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *      Gyroscope sensor definition
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *      Enrico Joerns <e.joerns@tu-bs.de>
 */

/**
 * \addtogroup inga_sensors
 * @{
 */

/**
 * \defgroup inga_gyro_driver Gyroscope Sensor
 *
 * This sensor interface allows to control the gyroscope of the INGA sensor platform. 
 *
 * @note This interface provides only a subset of the full sensor features.
 * If you require extended configuration, consider using the underlying sensor driver.
 *
 * \section cfg_sensor Configure sensor
 * <code> acc_sensor.configure(type, value) </code>
 *
 * The sensor interface allows to configure:
 * - Sensitivity (\ref GYRO_CONF_SENSITIVITY)
 * - Data rate (\ref GYRO_CONF_DATA_RATE)
 *
 * Details of the different configurations are given in
 * the documentation of respective config.
 *
 * \section query_measure Query measurements
 * <code>acc_sensor.value(channel)</code>
 *
 * The sensor provides 7 data channels.
 * - 3 provide dps values
 * - 3 provide unprocessed raw values
 * - 1 provides temperature value
 *
 * @note Measurement values are updated only if one channel
 * is read twice. I.e. reading x,y, and z channel
 * will provide values from a single measurement.
 * If then one of these channels is read again, a new
 * measurement is initated.
 *
 * \section usage Example Usage
 *
\code
#include <sensors.h>
[...]
struct sensors_sensor gyrosensor = find_sensor("Gyro");
ACTIVATE_SENSOR(gyrosensor);
gyrosensor.configure(GYRO_CONF_SENSITIVITY, GYRO_250DPS);
[...]
int x = gyrosensor.value(GYRO_X);
int y = gyrosensor.value(GYRO_Y);
int z = gyrosensor.value(GYRO_Z);
\endcode
 *
 * @{
 */

#ifndef __GYRO_SENSOR_H__
#define __GYRO_SENSOR_H__

#include "lib/sensors.h"

extern const struct sensors_sensor gyro_sensor;

#define GYRO_SENSOR "Gyro"

/** 
 * \name Data Output Channels
 * GYRO_X/Y/Z provide values for the respective axis in degree pre seconds (dps).
 * GYRO_X/Y/Z_RAW provide raw output values.
 * @{ */
#define GYRO_X      0
#define GYRO_Y      1
#define GYRO_Z      2
#define GYRO_X_RAW  3
#define GYRO_Y_RAW  4
#define GYRO_Z_RAW  5
#define GYRO_TEMP   6
/** @} */

/**
 * \name Configuration Types
 *
 * Sensor specific configuration types.
 * @{ */
/** Configures the sensitivity.
 * The sensor can operate at three different sensitivity levels (+/-250,+/-500,+/-2000 [dps]).
 * Possible values can be found in \ref sens_val "SENSITIVITY Values"
 */
#define GYRO_CONF_SENSITIVITY  10
/** Configures the output data rate.
 * The sensor can operate at different update rates (100,200,400,800 [Hz]).
 * Allowed values can be found in \ref rate_val "DATA_RATE Values"
 */
#define GYRO_CONF_DATA_RATE    20
/** @} */


/**
 * \name SENSITIVITY Values
 * \anchor sens_val
 * \see GYRO_CONF_SENSITIVITY
 * @{ */
#define GYRO_250DPS  250
#define GYRO_500DPS  500
#define GYRO_2000DPS 2000
/** @} */


/** 
 * \name DATA_RATE Values.
 * \anchor rate_val
 * \see GYRO_CONF_DATA_RATE
 * @{
 */
#define GYRO_100HZ  100
#define GYRO_200HZ  200
#define GYRO_400HZ  400
#define GYRO_800HZ  800
/** @} */

#endif /* __GYRO-SENSOR_H__ */

/** @} */ // inga_gyro_driver
/** @} */ // inga_sensors
