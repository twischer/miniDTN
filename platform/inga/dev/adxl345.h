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
 *		ADXL345 Accelerometer interface definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_sensors_driver
 * @{
 */

/**
 * \defgroup adxl345_interface ADXL345 Accelerometer Interface
 * @{
 */

#ifndef ADXL345_H_
#define ADXL345_H_

#include "../dev/mspi.h"
#include <stdio.h>
#include <util/delay.h>

/*!
 * SPI device order. The chip select number where the
 * ADXL345 is connected to the BCD-decimal decoder
 */
#define ADXL345_CS 					2

/*!
 * ADXL Data Format Register
 */
#define ADXL345_DATA_FORMAT_REG		0x31
/*!
 * ADXL Data Format Register Data:
 *<table border="1">
 * <tr>
 * <th> D7 </th>
 * <th> D6 </th>
 * <th> D5 </th>
 * <th> D4 </th>
 * <th> D3 </th>
 * <th> D2 </th>
 * <th> D1 </th>
 * <th> D0 </th>
 * </tr>
 * <tr>
 * <td> SELF_TEST </td>
 * <td> SPI </td>
 * <td> INT_INVERT </td>
 * <td> 0 </td>
 * <td> FULL_RES </td>
 * <td> Justify </td>
 * <td> Range </td>
 * <td> Range </td>
 * </tr>
 * </table>
 * \note For further information use the ADXL345 Datasheet
 * \note Default value: 0x00
 */
#define ADXL345_DATA_FORMAT_DATA	0x00

/*!
 * ADXL Power Control Register
 */
#define ADXL345_POWER_CTL_REG		0x2D
/*!
 * ADXL Power Control Register Data:
 *<table border="1">
 * <tr>
 * <th> D7 </th>
 * <th> D6 </th>
 * <th> D5 </th>
 * <th> D4 </th>
 * <th> D3 </th>
 * <th> D2 </th>
 * <th> D1 </th>
 * <th> D0 </th>
 * </tr>
 * <tr>
 * <td> 0 </td>
 * <td> 0 </td>
 * <td> Link </td>
 * <td> Auto Sleep </td>
 * <td> Measure </td>
 * <td> Sleep </td>
 * <td> Wakeup </td>
 * <td> Wakeup </td>
 * </tr>
 * </table>
 * \note For further information use the ADXL345 Datasheet
 * \note Default value: 0x00
 */
#define ADXL345_POWER_CTL_DATA		0x08


#define ADXL345_FIFO_CTL_REG		0x38


/*!
 * ADXL Data Rate and Power Mode Control Register
 *<table border="1">
 * <tr>
 * <th> D7 </th>
 * <th> D6 </th>
 * <th> D5 </th>
 * <th> D4 </th>
 * <th> D3 </th>
 * <th> D2 </th>
 * <th> D1 </th>
 * <th> D0 </th>
 * </tr>
 * <tr>
 * <td> 0 </td>
 * <td> 0 </td>
 * <td> 0 </td>
 * <td> LOW_POWER </td>
 * <td> Rate_3 </td>
 * <td> Rate_2 </td>
 * <td> Rate_1 </td>
 * <td> Rate_0 </td>
 * </tr>
 * </table>
 * \note For further information use the ADXL345 Datasheet
 * \note Default value: 0x0A */
#define ADXL345_BW_RATE_REG       0x2C
/*!
 * ADXL Data Rate and Power Mode Control Register Data:
 */
#define ADXL345_BW_RATE_DATA      0x0A

/*Device ID Register and value*/
#define ADXL345_DEVICE_ID_REG     0x00
#define ADXL345_DEVICE_ID_DATA		0xE5
/*Acceleration Data register (high/low)*/
#define ADXL345_OUTX_LOW_REG      0x32
#define ADXL345_OUTX_HIGH_REG  		0x33
#define ADXL345_OUTY_LOW_REG      0x34
#define ADXL345_OUTY_HIGH_REG     0x35
#define ADXL345_OUTZ_LOW_REG      0x36
#define ADXL345_OUTZ_HIGH_REG     0x37

#define ADXL345_FIFO_CTL_REG      0x38
#define ADXL345_FIFO_STATUS_REG		0x39

// BW_RATE
#define ADXL345_LOW_POWER     4
#define ADXL345_RATE_L        0

// DATA_FORMAT
#define ADXL345_SELF_TEST     7
#define ADXL345_SPI           6
#define ADXL345_INT_INVERT		5
#define ADXL345_FULL_RES      3
#define ADXL345_JUSTIFY       2
#define ADXL345_RANGE_H       1
#define ADXL345_RANGE_L       0

// FIFO_CTL
#define ADXL345_FIFO_MODE_L		6
#define ADXL345_TRIGGER       5
#define ADXL345_SAMPLES_L     0

// FIFO_STATUS
#define ADXL345_FIFO_TRIG     7
#define ADXL345_ENTRIES_L     0

#define ADXL345_RANGE_L       0


#define ADXL345_MODE_2G				(0x0 << ADXL345_RANGE_L)
#define ADXL345_MODE_4G				(0x1 << ADXL345_RANGE_L)
#define ADXL345_MODE_8G				(0x2 << ADXL345_RANGE_L)
#define ADXL345_MODE_16G			(0x3 << ADXL345_RANGE_L)

#define ADXL345_MODE_BYPASS		(0x0 << ADXL345_FIFO_MODE_L)
#define ADXL345_MODE_FIFO			(0x1 << ADXL345_FIFO_MODE_L)
#define ADXL345_MODE_STREAM		(0x2 << ADXL345_FIFO_MODE_L)
#define ADXL345_MODE_TRIGGER	(0x3 << ADXL345_FIFO_MODE_L)

/**
 * \name Values for output data rate
 * \see ADXL345_BW_RATE_REG
 * \{
 */
/// ODR: 0.1 Hz, bandwith: 0.05Hz, I_DD: 23 µA
#define ADXL345_ODR_0HZ10			(0x0 << ADXL345_RATE_L)
/// ODR: 0.2 Hz, bandwith: 0.1Hz, I_DD: 23 µA
#define ADXL345_ODR_0HZ20			(0x1 << ADXL345_RATE_L)
/// ODR: 0.39 Hz, bandwith: 0.2Hz, I_DD: 23 µA
#define ADXL345_ODR_0HZ39			(0x2 << ADXL345_RATE_L)
/// ODR: 0.78 Hz, bandwith: 0.39Hz, I_DD: 23 µA
#define ADXL345_ODR_0HZ78			(0x3 << ADXL345_RATE_L)
/// ODR: 1.56 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_1HZ56			(0x4 << ADXL345_RATE_L)
/// ODR: 3.13 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_3HZ13			(0x5 << ADXL345_RATE_L)
/// ODR: 6.25 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_6HZ25			(0x6 << ADXL345_RATE_L)
/// ODR: 12.5 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_12HZ5			(0x7 << ADXL345_RATE_L)
/// ODR: 25 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_25HZ			(0x8 << ADXL345_RATE_L)
/// ODR: 50 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_50HZ			(0x9 << ADXL345_RATE_L)
/// ODR: 100 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_100HZ			(0xA << ADXL345_RATE_L)
/// ODR: 200 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_200HZ			(0xB << ADXL345_RATE_L)
/// ODR: 400 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_400HZ			(0xC << ADXL345_RATE_L)
/// ODR: 800 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_800HZ			(0xD << ADXL345_RATE_L)
/// ODR: 1600 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_1600HZ		(0xE << ADXL345_RATE_L)
/// ODR: 3200 Hz, bandwith: x.xxHz, I_DD: xx µA
#define ADXL345_ODR_3200HZ		(0xF << ADXL345_RATE_L)
/** \} */

typedef struct {
  int16_t acc_x_value;
  int16_t acc_y_value;
  int16_t acc_z_value;
} acc_data_t;

/**
 * \brief Initialize the ADXL345 Acceleration Sensor
 *
 * The specific sensor settings are provided by
 * <ul>
 *  <li> ADXL345_DATA_FORMAT_DATA
 *  <li> ADXL345_POWER_CTL_DATA
 *  <li> ADXL345_BW_RATE_DATA
 * </ul>
 * \return 	<ul>
 *  		<li> 0 ADXL345 available
 *  		<li> -1 ADXL345 not available
 * 		 	</ul>
 */
int8_t adxl345_init(void);
/**
 * Checks whether the device is ready or not by reading the ID.
 * @return 0 if ready, otherwise -1
 */
int8_t adxl345_ready(void);
/**
 * 
 * @param range
 */
inline void adxl345_set_g_range(uint8_t range);
/**
 * 
 * @param mode
 */
void adxl345_set_fifomode(uint8_t mode);
/**
 * 
 * @param mode
 * @todo unimplemented
 */
void adxl345_set_powermode(uint8_t mode);

/**
 * \brief This function returns the current measured acceleration
 * at the x-axis of the adxl345
 *
 * \return current x-axis acceleration value
 */
int16_t adxl345_get_x_acceleration(void);

/**
 * \brief This function returns the current measured acceleration
 * at the y-axis of the adxl345
 *
 * \return current y-axis acceleration value
 */
int16_t adxl345_get_y_acceleration(void);

/**
 * \brief This function returns the current measured acceleration
 * at the z-axis of the adxl345
 *
 * \return current z-axis acceleration value
 */
int16_t adxl345_get_z_acceleration(void);


/**
 * \brief This function returns the current measured acceleration
 * of all axis (x,y,z)
 * \note This is more efficient than reading each axis individually
 *
 * \return current acceleration value of all axis
 */
acc_data_t adxl345_get_acceleration(void);

/**
 * \brief This function writes data to the given register
 *        of the ADXL345
 * \param reg  The register address
 * \param data The data value
 */
void adxl345_write(uint8_t reg, uint8_t data);

/**
 * \brief This function reads from the given register
 *        of the ADXL345
 * \param reg   The register address
 * \return      The data value
 */
uint8_t adxl345_read(uint8_t reg);

/** @} */
/** @} */

#endif /* ADXL345_H_ */
