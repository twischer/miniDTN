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
 * @{
 *
 * \defgroup Bosch BMP085 Digital Pressure Sensor
 *
 * <p>With an pressure sensor it is possible to measure the activity of
 * a person. Especially the Bosch BMP085 allows the registration of small
 * height fluctuations by sensing the air pressure. In some related papers,
 * a concrete application was implemented, to show how a fall analysis could
 * be done</p>
 *
 * @{
 *
 */

/**
 * \file
 *		Bosch BMP085 Digital Pressure Sensor
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */
#include "../drv/i2c-drv.h"
#include <util/delay.h>

#ifndef PRESSUREBMP085_H_
#define PRESSUREBMP085_H_
/*Pressure Sensor BMP085 device address*/
/*!
 * I2C address to read data
 */
#define BMP085_DEV_ADDR_R		0xEF
/*!
 * I2C address to write data
 */
#define BMP085_DEV_ADDR_W		0xEE
/*Control register address*/
/*!
 * Basic control register address
 */
#define BMP085_CTRL_REG_ADDR	0xF4
/*!
 * Control register address for temperature
 */
#define BMP085_CTRL_REG_TEMP	0x2E
/*!
 * Control register address for oversampling mode 0
 */
#define BMP085_CTRL_REG_PRESS_0	0x34
/*!
 * Control register address for oversampling mode 1
 */
#define BMP085_CTRL_REG_PRESS_1	0x74
/*!
 * Control register address for oversampling mode 2
 */
#define BMP085_CTRL_REG_PRESS_2	0xB4
/*!
 * Control register address for oversampling mode 3
 */
#define BMP085_CTRL_REG_PRESS_3	0xF4
/*Data register*/
/*!
 * Main data register address
 */
#define BMP085_DATA_REG_N		0xF6
/*!
 * Extended data register address for 19bit
 * resolution
 */
#define BMP085_DATA_REG_X		0xF8
/*EEPROM Register addresses for calibration data*/
/*!
 * Coefficient AC1 eeprom address
 */
#define BMP085_AC1_ADDR 		0xAA
/*!
 * Coefficient AC2 eeprom address
 */
#define BMP085_AC2_ADDR			0xAC
/*!
 * Coefficient AC3 eeprom address
 */
#define BMP085_AC3_ADDR			0xAE
/*!
 * Coefficient AC4 eeprom address
 */
#define BMP085_AC4_ADDR			0xB0
/*!
 * Coefficient AC5 eeprom address
 */
#define BMP085_AC5_ADDR			0xB2
/*!
 * Coefficient AC6 eeprom address
 */
#define BMP085_AC6_ADDR			0xB4
/*!
 * Coefficient B1 eeprom address
 */
#define BMP085_B1_ADDR			0xB6
/*!
 * Coefficient B2 eeprom address
 */
#define BMP085_B2_ADDR			0xB8
/*!
 * Coefficient MB eeprom address
 */
#define BMP085_MB_ADDR			0xBA
/*!
 * Coefficient MC eeprom address
 */
#define BMP085_MC_ADDR			0xBC
/*!
 * Coefficient MD eeprom address
 */
#define BMP085_MD_ADDR			0xBE
/*!
 * Hold all coefficients
 */
typedef struct {
	volatile int16_t ac1;
	volatile int16_t ac2;
	volatile int16_t ac3;
	volatile uint16_t ac4;
	volatile uint16_t ac5;
	volatile uint16_t ac6;
	volatile int16_t b1;
	volatile int16_t b2;
	volatile int16_t mb;
	volatile int16_t mc;
	volatile int16_t md;
} bmp085_calib_data;
/*!
 * Hold all coefficients
 */
static bmp085_calib_data bmp085_coeff;

/**
 * \brief Initialize the MPL115A Pressure Sensor
 *
 * \return <ul>
 *  	   <li> 0 at45db available
 *  	   <li> -1 at45db not available
 * 		   </ul>
 */
int8_t bmp085_init(void);

/**
 * \brief This functions reads the raw value of one
 * temperature conversion
 *
 * \return Temperature raw value (not compensated)
 */
int32_t bmp085_read_temperature(void);

/**
 * \brief This function reads the raw temperature value and
 * converts it to real temerature
 *
 * return Temerature in 0.1 Degree
 */
int32_t bmp085_read_comp_temperature(void);
/**
 * \brief This functions reads the raw value of one
 * pressure conversion.
 *
 * \param mode <table border="1">
 * <tr>
 * <th> 0 </th>
 * <th> 1 </th>
 * <th> 2 </th>
 * <th> 3 </th>
 * </tr>
 * <tr>
 * <td> ultra low power </td>
 * <td> standard  </td>
 * <td> high resolution  </td>
 * <td> ultra high resolution </td>
 * </tr>
 * </table>
 *
 * \return Temperature raw value (not compensated)
 */
int32_t bmp085_read_pressure(uint8_t mode);

/**
 * \brief This functions reads the temperature compensated
 *  value of one pressure conversion
 *
 *
 * \param mode <table border="1">
 * <tr>
 * <th> 0 </th>
 * <th> 1 </th>
 * <th> 2 </th>
 * <th> 3 </th>
 * </tr>
 * <tr>
 * <td> ultra low power </td>
 * <td> standard  </td>
 * <td> high resolution  </td>
 * <td> ultra high resolution </td>
 * </tr>
 * </table>
 *
 * \return Temperature raw value (not compensated)
 */
int32_t bmp085_read_comp_pressure(uint8_t mode);

/**
 * \brief Reads all coefficients from eeprom
 */
void bmp085_read_calib_data(void);

/**
 * \brief Generic minor function to read two byte
 * sequentially
 *
 * \return 16Bit (two Byte) data from bmp085
 */
uint16_t bmp085_read16bit_data(uint8_t addr);

/**
 * \brief Generic minor function to read one byte
 *
 * \return 8Bit (one Byte) data from bmp085
 */
uint8_t bmp085_read8bit_data(uint8_t addr);

#endif /* PRESSUREBMP085_H_ */
