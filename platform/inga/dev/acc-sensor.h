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
 *      Accelerometer sensor definition
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *      Enrico Jörns <joerns@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_sensors
 * @{
 */

/** 
 * \defgroup inga_acc_driver Accelerometer Sensor
 * 
 * This sensor interface allows to control the accelerometer of the INGA sensor platform.
 * 
 * @note This interface provides only a subset of the full sensor features.
 * If you require extended configuration, consider using the underlying sensor driver.
 *
 *
 * \section act_acc Activate Sensor
 * 
 * Default initialization settings:
 *  - <b>bypass mode</b>, i.e. a read command will always return the latest sampled data
 *  - 100Hz sample rate
 * 
 *
 * \section cfg_sensor Configure sensor
 * <code> acc_sensor.configure(type, value) </code>
 *
 * The sensor interface allows to configure:
 * - Sensitivity (\ref ACC_CONF_SENSITIVITY)
 * - Data rate (\ref ACC_CONF_DATA_RATE)
 *
 * Details of the different configurations are given in
 * the documentation of respective config.
 *
 *
 * \section query_measure Query measurements
 * <code>acc_sensor.value(channel)</code>
 *
 * The sensor provides 6 data channels.
 * - 3 provide mg values
 * - 3 provide unprocessed raw values
 * A detailed list can be found at \ref acc_sensor_channels "Channels"
 * 
 * @note Measurement values are updated only if one channel
 * is read twice. I.e. reading x,y, and z channel
 * will provide values from a single measurement.
 * If then one of these channels is read again, a new
 * measurement is initated.
 * 
 *
 * \section usage Example Usage
 *
\code
#include <sensors.h>
[...]
struct sensors_sensor accsensor = find_sensor("ACC");
ACTIVATE_SENSOR(accsensor);
accsensor.configure(GYRO_CONF_SENSITIVITY, ACC_25HZ);
[...]
int x = accsensor.value(ACC_X);
int y = accsensor.value(ACC_Y);
int z = accsensor.value(ACC_Z);
\endcode
 *
 * @{ */

#ifndef __ADXL345_SENSOR_H__
#define __ADXL345_SENSOR_H__

#include "lib/sensors.h"

extern const struct sensors_sensor acc_sensor;

#define ACC_SENSOR "Acc"

/**
 * \name Data Output Channels
 * \anchor acc_sensor_channels
 * 
 * @{ */
/** x axis data (mg output) */
#define ACC_X       0
/** y axis data (mg output) */
#define ACC_Y       1
/** z axis data (mg output) */
#define ACC_Z       2
/** x axis data (raw output) */
#define ACC_X_RAW   3
/** y axis data (raw output) */
#define ACC_Y_RAW   4
/** z axis data (raw output) */
#define ACC_Z_RAW   5
/** @} */

/**
 * \name Configuration types
 * @{ */
/**
 * Sensitivity configuration
 * The sensor can operate at different ranges at different sensitivity levels (+/-2g,+/-4g,+/-8g,+/-16g)
 * Possible values can be found in \ref sens_val "SENSITIVITY Values"
 */
#define ACC_CONF_SENSITIVITY  10
/**
 * FIFO mode configuration 
 * 
 * - \ref ACC_MODE_BYPASS - Bypass mode
 * - \ref ACC_MODE_FIFO		- FIFO mode
 * - \ref ACC_MODE_STREAM - Stream mode
 */
#define ACC_CONF_FIFOMODE     20
/**
 * Controls power / sleep mode
 *
 * - \ref ACC_NOSLEEP -	No sleep
 * - \ref ACC_AUTOSLEEP - Autosleep
 */
#define ACC_CONF_POWERMODE    30
/**
 * Output data rate.
 * 
 * Values should be one of those described in
 * \ref acc_sensor_odr_values "Output Data Rate Values".
 * 
 * \note If another value is given, the value is rounded to the next upper valid value.
 * 
 * \note In bypass mode (default) you should always select twice the rate of your
 * desired readout frequency
 * 
 */
#define ACC_CONF_DATA_RATE           40
/** @} */

/**
 * \name Status types
 * @{ */
/**
 * Fill level of the sensor buffer (If working in FIFO or Streaming mode).
 * Max value: 33 (full)
 * 
 * This can be used to determine whether a buffer overflow occured between
 * two readouts or to adapt readout rate.
 */
#define ACC_STATUS_BUFFER_LEVEL     50
/** @} */


/**
 * \name SENSITIVITY Values
 * \anchor sens_val
 * \see ACC_SENSOR_SENSITIVITY
 * @{ */
#define ACC_2G  2
#define ACC_4G  4
#define ACC_8G  8
#define ACC_16G 16
/** @} */

/**
 * \name Output Data Rate Values
 * \anchor acc_sensor_odr_values
 * \{
 */
/// ODR: 0.1 Hz, bandwith: 0.05Hz, I_DD: 23 µA
#define ACC_0HZ10			1
/// ODR: 0.2 Hz, bandwith: 0.1Hz, I_DD: 23 µA
#define ACC_0HZ20			2
/// ODR: 0.39 Hz, bandwith: 0.2Hz, I_DD: 23 µA
#define ACC_0HZ39			4
/// ODR: 0.78 Hz, bandwith: 0.39Hz, I_DD: 23 µA
#define ACC_0HZ78			8
/// ODR: 1.56 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_1HZ56			16
/// ODR: 3.13 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_3HZ13			31
/// ODR: 6.25 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_6HZ25			63
/// ODR: 12.5 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_12HZ5			125
/// ODR: 25 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_25HZ			250
/// ODR: 50 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_50HZ			500
/// ODR: 100 Hz, bandwith: x.xxHz, I_DD: xx µA (default)
#define ACC_100HZ			1000
/// ODR: 200 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_200HZ			2000
/// ODR: 400 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_400HZ			4000
/// ODR: 800 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_800HZ			8000
/// ODR: 1600 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_1600HZ		16000
/// ODR: 3200 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ACC_3200HZ		32000
/** \} */

/**
 * \name FIFO mode values
 * \see ACC_SENSOR_FIFOMODE
 * @{ */
/** Bypass mode (default).
 * No buffering, data is always the latest.
 */
#define ACC_MODE_BYPASS  0x0
/** FIFO mode.
 *  A 32 entry fifo is used to buffer unread data.
 *  If buffer is full, no data is read anymore.
 */
#define ACC_MODE_FIFO    0x1
/** Stream mode.
 *  A 32 entry fifo is used to buffer unread data.
 *  If buffer is full old data will be dropped and replaced by new one.
 */
#define ACC_MODE_STREAM  0x2
/** @} */


/**
 * \name Power mode values
 * \see ACC_SENSOR_POWERMODE
 * @{ */
/** No sleep */
#define ACC_NOSLEEP   0
/** Auto sleep */
#define ACC_AUTOSLEEP 1
/** @} */

#endif /* __ACC-SENSOR_H__ */

/** @} */
/** @} */

