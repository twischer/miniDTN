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
 * @{
 */

#ifndef __GYRO_SENSOR_H__
#define __GYRO_SENSOR_H__

#include "lib/sensors.h"
#include "l3g4200d.h"

/**
 * \name Configurations
 * @{
 */
/** Configures the sensitivity. */
#define GYRO_CONF_SENSITIVITY  10
/** Configures the output data rate. */
#define GYRO_CONF_DATA_RATE    20
/** Configures the fifo mode. */
#define GYRO_CONF_FIFOMODE     30
/** @} */

/**
 * \name Sensitivity Values
 * \see GYRO_SENSITIVITY
 * @{ */
/// 250 dps
#define GYRO_250DPS  250
/// 500 dps
#define GYRO_500DPS  500
/// 2000 dps
#define GYRO_2000DPS 2000
/** @} */


/** \name Data rate Values.
 *  \see GYRO_DATARATE
 * @{
 */
#define GYRO_100HZ  100
#define GYRO_200HZ  200
#define GYRO_400HZ  400
#define GYRO_800HZ  800
/** @} */

extern const struct sensors_sensor gyro_sensor;

#define GYRO_X      0
#define GYRO_Y      1
#define GYRO_Z      2
#define GYRO_X_RAW  3
#define GYRO_Y_RAW  4
#define GYRO_Z_RAW  5
#define GYRO_TEMP   6

#endif /* __GYRO-SENSOR_H__ */

/** @} */ // inga_gyro_driver
/** @} */ // inga_sensors
