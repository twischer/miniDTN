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
 *		Bosch BMP085 Digital Pressure Sensor driver definition
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_sensors_driver
 * @{
 */

/**
 * \defgroup bmp085_interface BMP085 Digital Pressure Sensor
 *
 * With a pressure sensor it is possible to measure the activity of
 * a person. Especially the Bosch BMP085 allows the registration of small
 * height fluctuations by sensing the air pressure. In some related papers,
 * a concrete application was implemented, to show how a fall analysis could
 * be done.
 * @{
 */

#ifndef PRESSUREBMP085_H_
#define PRESSUREBMP085_H_

#include "../dev/i2c.h"
#include <util/delay.h>

/**
 * Tests if BMP085 is available
 * \retval 1 is avaiable
 * \retval 0 is not available
 */
int8_t bmp085_available(void);

/**
 * \brief Initializes the BMP085 Pressure Sensor
 *
 * \retval 0 bmp085 available
 * \retval 1 bmp085 not available
 */
int8_t bmp085_init(void);

/**
 * \brief Deinitializes the sensor.
 * @todo: Dummy function that does nothing
 */
int8_t bmp085_deinit(void);

/**
 * \brief This function reads and returns the calibration compensated temerature
 *
 * \return Temerature in 0.1 Degree
 */
int16_t bmp085_read_temperature(void);

/**
 * \name Operation modes
 * @{
 */
/// Ultra low power mode
#define BMP085_ULTRA_LOW_POWER 0
/// Standard mode
#define BMP085_STANDARD        1
/// High resolution mode
#define BMP085_HIGH_RESOLUTION 2
/// Ultra high resolution mode
#define BMP085_ULTRA_HIGH_RES  3
/** @}*/

/**
 * \brief This functions reads the temperature compensated
 *  value of one pressure conversion
 *
 * \param mode one of
 * \ref BMP085_ULTRA_LOW_POWER,
 * \ref BMP085_STANDARD,
 * \ref BMP085_HIGH_RESOLUTION and
 * \ref BMP085_ULTRA_HIGH_RES
 * 
 * \return Pressure in Pa
 */
int32_t bmp085_read_pressure(uint8_t mode);

#endif /* PRESSUREBMP085_H_ */

/** @} */
/** @} */
