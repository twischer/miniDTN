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
 *
 * \defgroup mpl115a_interface Pressure Sensor (Barometer) MPL115A
 *
 * <p> A pressure sensor sounds a little bit strange in conjunction with
 * a health care sensor node, but it exists an interesting task. With the help
 * of the differential pressure value, it is possible to recognize an up and
 * down movement of a test person.
 * Therefore the Freescale MPL115A (SPI) was integrated onto the sensor node. This
 * pressure sensor has a very small design size and furthermore an independent
 * temperature sensor. In Combination with the temperature value and a few coefficients,
 * the raw pressure value can be compensated to get the exact result. The coefficients
 * are stored in the internal ROM of the MPL115A and can be read out via SPI. The
 * compensation can be done by the MCU (this option is implemented), or by an external
 * application (e.g. server application)
 * \note The pressure compensation costs about 6 32Bit multiplications
 * </p>
 * @{
 *
 */

/**
 * \file
 *		MPL115A Pressure Sensor (Barometer) interface definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */
#include "../drv/mspi-drv.h"
#include <util/delay.h>

#ifndef PRESSUREMPL115A_H_
#define PRESSUREMPL115A_H_

/*!
 * SPI device order. The chip select number where the
 * pressure sensor mpl115a is connected to the BCD-decimal
 * decoder
 */
#define MPL115A_CS					4

/*!
 * Pressure value high byte
 * <table border="1">
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
 * <td> Bit9 </td>
 * <td> Bit8 </td>
 * <td> Bit7 </td>
 * <td> Bit6 </td>
 * <td> Bit5 </td>
 * <td> Bit4 </td>
 * <td> Bit3 </td>
 * <td> Bit2 </td>
 * </tr>
 * </table>
 */
#define MPL115A_PRESSURE_OUT_MSB	0x80 | (0x00 << 1)
/*!
 * Pressure value low byte
 * <table border="1">
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
 * <td> Bit1 </td>
 * <td> Bit0 </td>
 * <td> x </td>
 * <td> x </td>
 * <td> x </td>
 * <td> x </td>
 * <td> x </td>
 * <td> x </td>
 * </tr>
 * </table>
 */
#define MPL115A_PRESSURE_OUT_LSB	0x80 | (0x01 << 1)

/*!
 * Temperature value high byte
 * <table border="1">
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
 * <td> Bit9 </td>
 * <td> Bit8 </td>
 * <td> Bit7 </td>
 * <td> Bit6 </td>
 * <td> Bit5 </td>
 * <td> Bit4 </td>
 * <td> Bit3 </td>
 * <td> Bit2 </td>
 * </tr>
 * </table>
 */
#define MPL115A_TEMP_OUT_MSB		0x80 | (0x02 << 1)
/*!
 * Temperature value low byte
 * <table border="1">
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
 * <td> Bit1 </td>
 * <td> Bit0 </td>
 * <td> x </td>
 * <td> x </td>
 * <td> x </td>
 * <td> x </td>
 * <td> x </td>
 * <td> x </td>
 * </tr>
 * </table>
 */
#define MPL115A_TEMP_OUT_LSB		0x80 | (0x03 << 1)

/*!
 * Pressure compensate coefficient A0 (high byte)
 */
#define MPL115A_COEFF_ADDR_A0_M		0x80 | (0x04 << 1)
/*!
 * Pressure compensate coefficient A0 (low byte)
 */
#define MPL115A_COEFF_ADDR_A0_L		0x80 | (0x05 << 1)

/*!
 * Pressure compensate coefficient B1 (high byte)
 */
#define MPL115A_COEFF_ADDR_B1_M		0x80 | (0x06 << 1)
/*!
 * Pressure compensate coefficient B1 (low byte)
 */
#define MPL115A_COEFF_ADDR_B1_L		0x80 | (0x07 << 1)

/*!
 * Pressure compensate coefficient B2 (high byte)
 */
#define MPL115A_COEFF_ADDR_B2_M		0x80 | (0x08 << 1)
/*!
 * Pressure compensate coefficient B2 (low byte)
 */
#define MPL115A_COEFF_ADDR_B2_L		0x80 | (0x09 << 1)

/*!
 * Pressure compensate coefficient C12 (high byte)
 */
#define MPL115A_COEFF_ADDR_C12_M	0x80 | (0x0A << 1)
/*!
 * Pressure compensate coefficient C12 (low byte)
 */
#define MPL115A_COEFF_ADDR_C12_L	0x80 | (0x0B << 1)

/*!
 * Pressure compensate coefficient C11 (high byte)
 */
#define MPL115A_COEFF_ADDR_C11_M	0x80 | (0x0C << 1)
/*!
 * Pressure compensate coefficient C11 (low byte)
 */
#define MPL115A_COEFF_ADDR_C11_L	0x80 | (0x0D << 1)

/*!
 * Pressure compensate coefficient C22 (high byte)
 */
#define MPL115A_COEFF_ADDR_C22_M	0x80 | (0x0E << 1)
/*!
 * Pressure compensate coefficient C22 (low byte)
 */
#define MPL115A_COEFF_ADDR_C22_L	0x80 | (0x0F << 1)

/*!
 * Start pressure measurement command
 */
#define MPL115A_START_P_CONV		(0x10 << 1)
/*!
 * Start temperature measurement command
 */
#define MPL115A_START_T_CONV		(0x11 << 1)
/*!
 * Start both, pressure and temperature measurement
 * command
 */
#define MPL115A_START_B_CONV		(0x12 << 1)

#define MPL115A_A0					0
#define MPL115A_B1					1
#define MPL115A_B2					2
#define MPL115A_C12					3
#define MPL115A_C11					4
#define MPL115A_C22					5



typedef struct{
	/*!
	 * Pressure compensate coefficient value
	 */
	int16_t value;
	/*!
	 * coefficient addresses
	 */
	uint8_t addr[2];
}coeff_t;

/*!
 * Hold all coefficients with their belonging addresses
 */
static coeff_t coefficients[6] = {
		{0x000, {MPL115A_COEFF_ADDR_A0_M , MPL115A_COEFF_ADDR_A0_L}},
		{0x000, {MPL115A_COEFF_ADDR_B1_M , MPL115A_COEFF_ADDR_B1_L}},
		{0x000, {MPL115A_COEFF_ADDR_B2_M , MPL115A_COEFF_ADDR_B2_L}},
		{0x000, {MPL115A_COEFF_ADDR_C12_M , MPL115A_COEFF_ADDR_C12_L}},
		{0x000, {MPL115A_COEFF_ADDR_C11_M , MPL115A_COEFF_ADDR_C11_L}},
		{0x000, {MPL115A_COEFF_ADDR_C22_M , MPL115A_COEFF_ADDR_C22_L}}
};


/**
 * \brief Initialize the MPL115A Pressure Sensor
 *
 */
void mpl115a_init(void);

/**
 * \brief Get raw pressure value (10Bit)
 *
 * \return Pressure value
 *
 * \note The overall pressure measurement takes approximately
 * 3ms
 */
uint16_t mpl115a_get_pressure(void);

/**
 * \brief Get raw temperature value (10Bit)
 *
 * \return Temperature value
 *
 * \note The overall temperature measurement takes approximately
 * 3ms
 */
uint16_t mpl115a_get_temp(void);

/*
 * \brief This function calculates and returns the compensated
 * pressure value
 *
 * \return Compensated pressure value
 *
 */
int16_t mpl115a_get_Pcomp(void);

/*
 * \brief This function reads out all coefficients from the
 * MPL115A ROM
 *
 * \note It is not clear, if these values will change or if
 * they are fix, please check that
 *
 */
void mpl115a_read_coefficients(void);

/**
 * \brief This function reads and writes from the given
 * register of the MPL115A
 *
 * \param reg   The register address
 *
 * \return      The data value
 */
uint8_t mpl115a_cmd(uint8_t reg);

#endif /* PRESSUREMPL115A_H_ */
