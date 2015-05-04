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
 *      Pressure sensor interface definition
 * \author
 *      Enrico Joerns <joerns@ibr.cs.tu-bs.de>
 *      Georg von Zengen
 */

/**
 * \addtogroup inga_sensors
 * @{
 */

/** \defgroup inga_pressure_driver Pressure and Temperature Sensor
 *
 * This sensor interface allows to control the gyroscope of the INGA sensor platform.
 *
 * \section query_measure Query measurements
 * <code>acc_sensor.value(channel)</code>
 *
 * The sensor provides 3 data channels.
 * - 2 channels for pressure value (high/low)
 * - One channel for temperature
 *
 * \section usage Example Usage
 *
\code
#include <sensors.h>
[...]
struct sensors_sensor pressure_sensor = find_sensor("Press");
ACTIVATE_SENSOR(pressure_sensor);
[...]
int32_t pressval = ((int32_t) pressure_sensor.value(PRESS_H) << 16);
pressval |= (pressure_sensor.value(PRESS_L) & 0xFFFF);
\endcode
 * @{ */

#ifndef __PRESS_SENSOR_H__
#define __PRESS_SENSOR_H__

#include "lib/sensors.h"

extern const struct sensors_sensor pressure_sensor;

#define PRESSURE_SENSOR "Press"

/** \name Sensor values
 * @{ */
/** Returns Temperature [C] */
#define  TEMP     0
/** Returns Pressure [High] [Pa] */
#define  PRESS_H  2
/** Returns Pressure [Low] [Pa] */
#define  PRESS_L  1
/** @} */

/** \name Configuration types
 * @{ */
/** Selects the operation mode. */
#define PRESSURE_CONF_OPERATION_MODE  10
/** @} */

/** \name OPERATION_MODE values
 * \anchor op_mode_values
 * @{ */
#define PRESSURE_MODE_ULTRA_LOW_POWER 0
#define PRESSURE_MODE_STANDARD        1
#define PRESSURE_MODE_HIGH_RES        2
#define PRESSURE_MODE_ULTRA_HIGH_RES  3
/** @} */


/** @} */
/** @} */

#endif /* __PRESS-SENSOR_H__ */
