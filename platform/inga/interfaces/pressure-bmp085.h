/* Copyright (c) 2010, Ulf Kulau
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \addtogroup Device Interfaces
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
