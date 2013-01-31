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
#define GYRO_SENSITIVITY  10
/** Configures the output data rate. */
#define GYRO_DATA_RATE    20
/** Configures the fifo mode. */
#define GYRO_FIFOMODE     30
/** @} */

/**
 * \name Sensitivity Values
 * \see GYRO_SENSITIVITY
 * @{ */
/// 250 dps
#define GYRO_250DPS  L3G4200D_250DPS
/// 500 dps
#define GYRO_500DPS  L3G4200D_500DPS
/// 2000 dps
#define GYRO_2000DPS L3G4200D_2000DPS
/** @} */


/** \name Data rate Values.
 *  \see GYRO_DATARATE
 * @{
 */
#define GYRO_100HZ  L3G4200D_100HZ
#define GYRO_200HZ  L3G4200D_200HZ
#define GYRO_400HZ  L3G4200D_400HZ
#define GYRO_800HZ  L3G4200D_800HZ
/** @} */

extern const struct sensors_sensor gyro_sensor;

#define X_AS 0
#define Y_AS 1
#define Z_AS 2
#define TEMP_AS 3

#endif /* __GYRO-SENSOR_H__ */

/** @} */ // inga_gyro_driver
/** @} */ // inga_sensors
