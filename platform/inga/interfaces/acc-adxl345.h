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
 *
 * \section about About
 * The interfaces are the second layer above the hardware and aim to separate
 * the discrete hardware from the user application layer.
 *
 * \section usage Usage
 *
 * The interfaces are compatible with contiki os and can be directly used
 * in a process environment.
 *
 * \section lic License
 *
 * <pre>Copyright (c) 2009, Ulf Kulau <kulau@ibr.cs.tu-bs.de>
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
 * OTHER DEALINGS IN THE SOFTWARE.</pre>
 *
 * @{
 *
 * \defgroup adxl345_interface Accelerometer Interface (ADXL345)
 *
 * <p>In previous Projects we evaluated some accelerometer sensors and the digital ADXL345 is
 *    the best choice for an sensor node application. All accelerometers have nearly the same
 *    performance characteristics, but he ADXL345 is very cheap and has the lowest power
 *    consumption.
 *    </p>
 * @{
 *
 */

/**
 * \file
 *		ADXL345 Accelerometer interface definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

#ifndef ADXL345_H_
#define ADXL345_H_

#include "../drv/mspi-drv.h"
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
 */
#define ADXL345_BW_RATE_REG			0x2C
/*!
 * ADXL Data Rate and Power Mode Control Register Data:
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
 * \note Default value: 0x0A
 */
#define ADXL345_BW_RATE_DATA		0x0A

/*\cond*/
/*Device ID Register and value*/
#define ADXL345_DEVICE_ID_REG		0x00
#define ADXL345_DEVICE_ID_DATA		0xE5
/*Acceleration Data register (high/low)*/
#define ADXL345_OUTX_LOW_REG		0x32
#define ADXL345_OUTX_HIGH_REG  		0x33
#define ADXL345_OUTY_LOW_REG		0x34
#define ADXL345_OUTY_HIGH_REG		0x35
#define ADXL345_OUTZ_LOW_REG		0x36
#define ADXL345_OUTZ_HIGH_REG		0x37

/*\endcond*/

#define ADXL345_RANGE_L			0

#define ADXL345_MODE_2G				(0x0 << ADXL345_RANGE_L)
#define ADXL345_MODE_4G				(0x1 << ADXL345_RANGE_L)
#define ADXL345_MODE_8G				(0x2 << ADXL345_RANGE_L)
#define ADXL345_MODE_16G			(0x3 << ADXL345_RANGE_L)


typedef struct {
	uint16_t acc_x_value;
	uint16_t acc_y_value;
	uint16_t acc_z_value;
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

inline void adxl345_set_g_range(uint8_t range);
void adxl345_set_fifomode(uint8_t mode);
void adxl345_set_powermode(uint8_t mode);

/**
 * \brief This function returns the current measured acceleration
 * at the x-axis of the adxl345
 *
 * \return current x-axis acceleration value
 */
uint16_t adxl345_get_x_acceleration(void);

/**
 * \brief This function returns the current measured acceleration
 * at the y-axis of the adxl345
 *
 * \return current y-axis acceleration value
 */
uint16_t adxl345_get_y_acceleration(void);

/**
 * \brief This function returns the current measured acceleration
 * at the z-axis of the adxl345
 *
 * \return current z-axis acceleration value
 */
uint16_t adxl345_get_z_acceleration(void);


/**
 * \brief This function returns the current measured acceleration
 * of all axis (x,y,z)
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


#endif /* ADXL345_H_ */
