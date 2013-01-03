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
 * \addtogroup inga_driver
 * @{
 */

/** 
 * \defgroup inga_acc_driver ADXL345 Accelerometer Sensor Driver
 * @{
 */

/**
 * \file 
 *		Accelerometer driver
 * \author
 *		
 */

#ifndef __ADXL345_SENSOR_H__
#define __ADXL345_SENSOR_H__

#include "lib/sensors.h"

extern const struct sensors_sensor acc_sensor;

/**
 * \name Channels
 * @{ */
#define ADXL345_X   0
#define ADXL345_Y   1
#define ADXL345_Z   2
/** @} */

/**
 * \name Sensitivity Values
 * \see ADXL345_SENSOR_SENSITIVITY
 * @{ */
/// 2g acceleration
#define ADXL345_2G  0x0
/// 4g acceleration
#define ADXL345_4G  0x1
/// 8g acceleration
#define ADXL345_8G  0x2
/// 16g acceleration
#define ADXL345_16G 0x2
/** @} */

/**
 * \name FIFO mode values
 * \see ADXL345_SENSOR_FIFOMODE
 * @{ */
/// Bypass mode
#define ADXL345_BYPASS  0x0
/// FIFO mode
#define ADXL345_FIFO    0x1
/// Stream mode
#define ADXL345_STREAM  0x2
/// Trigger mode
#define ADXL345_TRIGGER 0x3
/** @} */

/**
 * \name Power mode values
 * \see ADXL345_SENSOR_POWERMODE
 * @{ */
/// No sleep
#define ADXL345_NOSLEEP   0
/// Auto sleep
#define ADXL345_AUTOSLEEP 1

/** @} */

/**
 * \name Setting types
 * @{ */
/**
  Sensitivity configuration
 
 - \ref ADXL345_2G   2g @256LSB/g
 - \ref ADXL345_4G   4g @128LSB/g
 - \ref ADXL345_8G   8g @64LSB/g
 - \ref ADXL345_16G 16g @32LSB/g
 */
#define ADXL345_SENSOR_SENSITIVITY  10
/**
 * FIFO mode configuration 
 * 
 * - \ref ADXL345_BYPASS  - Bypass mode
 * - \ref ADXL345_FIFO		- FIFO mode
 * - \ref ADXL345_STREAM  - Stream mode
 * - \ref ADXL345_TRIGGER - Trigger mode
 */
#define ADXL345_SENSOR_FIFOMODE     20
/**
 * Controls power / sleep mode
 * Value	Mode
 * - \ref ADXL345_NOSLEEP			No sleep
 * - \ref ADXL345_AUTOSLEEP			Autosleep
 */
#define ADXL345_SENSOR_POWERMODE    30
/** @} */

#endif /* __ACC-SENSOR_H__ */

/** @} */
/** @} */
