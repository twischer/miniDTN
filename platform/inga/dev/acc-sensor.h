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
 * \section Initialization
 * 
 * Default initialization settings:
 *  - <b>bypass mode</b>, i.e. a read command will always return the latest sampled data
 *  - 100Hz sample rate
 *  - 
 * 
 * \section acc_sensor_data_channels Data channels
 * The sensor provides 6 abstract data channels.
 * 3 for reading raw sensor data from x, y and z sensor axis
 * and 3 for reading sensor data in [mg] from x, y and z sensor axis.
 * A detailed list can be found at \ref acc_sensor_channels "Channels"
 * 
 * <code>acc_sensor.value(ACC_X)</code>
 * 
 * \section acc_sensor_settings Settings
 * 
 * \section acc_sensor_details Details
 * - The implementation allows reading values from the same sample, a new
 *   sample is taken only each time the \em same channel is read \em again.
 *   I.e. reading y,z,x,y will update data each time the y is read while
 *   reading z,z,z will update data at each readout.
 * 
 * @{
 */

/**
 * \file 
 *		Accelerometer driver
 * 
 * \author
 *    Enrico Joerns <e.joerns@tu-bs.de>
 *		
 */

#ifndef __ADXL345_SENSOR_H__
#define __ADXL345_SENSOR_H__

#include "lib/sensors.h"

extern const struct sensors_sensor acc_sensor;

/**
 * \name Channels
 * \anchor acc_sensor_channels
 * 
 * @{ */
/// x axis data (raw output)
#define ACC_X     0
/// y axis data (raw output)
#define ACC_Y     1
/// z axis data (raw output)
#define ACC_Z     2
/// x axis data (mg output)
#define ACC_X_MG  3
/// y axis data (mg output)
#define ACC_Y_MG  4
/// z axis data (mg output)
#define ACC_Z_MG  5
/// 
#define ACC_Z_MG  5
/** @} */

/**
 * \name Sensitivity Values
 * \see ACC_SENSOR_SENSITIVITY
 * @{ */
/// 2g acceleration
#define ACC_2G  2
/// 4g acceleration
#define ACC_4G  4
/// 8g acceleration
#define ACC_8G  8
/// 16g acceleration
#define ACC_16G 16
/** @} */

/**
 * \name Values for output data rate
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
/// ODR: 100 Hz, bandwith: x.xxHz, I_DD: xx µA
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
/// Bypass mode
#define ACC_BYPASS  0x0
/// FIFO mode
#define ACC_FIFO    0x1
/// Stream mode
#define ACC_STREAM  0x2
/// Trigger mode
#define ACC_TRIGGER 0x3
/** @} */


/**
 * \name Power mode values
 * \see ACC_SENSOR_POWERMODE
 * @{ */
/// No sleep
#define ACC_NOSLEEP   0
/// Auto sleep
#define ACC_AUTOSLEEP 1

/** @} */

/**
 * \name Setting types
 * <code> acc_sensor.configure(type, value) </code>
 * @{ */
/**
  Sensitivity configuration
 
 - \ref ACC_2G  -  2g @256LSB/g
 - \ref ACC_4G  -  4g @128LSB/g
 - \ref ACC_8G  -  8g @64LSB/g
 - \ref ACC_16G - 16g @32LSB/g
 */
#define ACC_CONF_SENSITIVITY  10
/**
 * FIFO mode configuration 
 * 
 * - \ref ACC_BYPASS  - Bypass mode
 * - \ref ACC_FIFO		- FIFO mode
 * - \ref ACC_STREAM  - Stream mode
 * - \ref ACC_TRIGGER - Trigger mode
 */
#define ACC_CONF_FIFOMODE     20
/**
 * Controls power / sleep mode
 *
 * - \ref ACC_NOSLEEP		-	No sleep
 * - \ref ACC_AUTOSLEEP	-	Autosleep
 */
#define ACC_CONF_POWERMODE    30
/**
 * Output data rate
 * 
 * Data rate is set to the upper next of twice the selectd rate.
 * Thus for pull, all data rates are valid.
 */
#define ACC_CONF_DATA_RATE           40
/** @} */

/** @} */
/** @} */

#endif /* __ACC-SENSOR_H__ */
