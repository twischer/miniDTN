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
 * \addtogroup mpl115a_interface Pressure Sensor (Barometer) MPL115A
 * @{
 */

/**
 * \file
 *		MPL115A Pressure Sensor (Barometer) interface implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

#include "pressure-mpl115a.h"

void mpl115a_init(void) {
	mspi_chip_release(MPL115A_CS);
	/*init mspi in mode0, at chip select pin 4 and max baud rate*/
	mspi_init(MPL115A_CS, MSPI_MODE_0, MSPI_BAUD_MAX);
}

uint16_t mpl115a_get_pressure(void) {
	uint8_t press_msb;
	uint8_t press_lsb;
	uint16_t pressure = 0x0000;
	/*start pressure measurement*/
	mpl115a_cmd(MPL115A_START_P_CONV);
	_delay_ms(6);
	press_msb = mpl115a_cmd(MPL115A_PRESSURE_OUT_MSB);
	press_lsb = mpl115a_cmd(MPL115A_PRESSURE_OUT_LSB);
	pressure = (uint16_t) (press_msb << 8);
	pressure &= (0xFF00 | ((uint16_t) (press_lsb)));
	pressure = 0x03FF & (pressure >> 6);
	return pressure;
}

uint16_t mpl115a_get_temp(void) {
	uint8_t temp_msb;
	uint8_t temp_lsb;
	uint16_t temperature = 0x0000;
	/*start temperature measurement*/
	mpl115a_cmd(MPL115A_START_T_CONV);
	_delay_ms(6);
	temp_msb = mpl115a_cmd(MPL115A_TEMP_OUT_MSB);
	temp_lsb = mpl115a_cmd(MPL115A_TEMP_OUT_LSB);

	temperature = (uint16_t) (temp_msb << 8);
	temperature &= (0xFF00 | ((uint16_t) (temp_lsb)));
	temperature = 0x03FF & (temperature >> 6);
	return temperature;
}

void mpl115a_read_coefficients(void) {
	uint8_t i;
	int8_t coeff_tmp_msb;
	int8_t coeff_tmp_lsb;

	for (i = 0; i < 6; i++) {
		coeff_tmp_msb = mpl115a_cmd(coefficients[i].addr[0]);
		coeff_tmp_lsb = mpl115a_cmd(coefficients[i].addr[1]);
		uart_TXchar(coeff_tmp_msb);
		uart_TXchar(coeff_tmp_lsb);


		coefficients[i].value = (int16_t) (coeff_tmp_msb << 8);
		coefficients[i].value &= (0xFF00 | ((int16_t) (coeff_tmp_lsb)));
	}
	uart_TXchar('\n');
}

int16_t mpl115a_get_Pcomp(void) {
	int32_t tmp1_val, tmp2_val, tmp3_val;
	int32_t tmp_help;
	uint8_t msb, lsb;
	uint16_t temperature = 0x0000;
	uint16_t pressure = 0x0000;

	/*start both, temperature and pressure measurement*/
	mpl115a_cmd(MPL115A_START_B_CONV);
	/*wait 3 ms*/
	_delay_ms(6);
	/*get temperature value*/
	msb = mpl115a_cmd(MPL115A_TEMP_OUT_MSB);
	lsb = mpl115a_cmd(MPL115A_TEMP_OUT_LSB);
	temperature = (uint16_t) (msb << 8);
	temperature &= (0xFF00 | ((uint16_t) (lsb)));
	temperature = 0x03FF & (temperature >> 6);
	/*get pressure value*/
	msb = mpl115a_cmd(MPL115A_PRESSURE_OUT_MSB);
	lsb = mpl115a_cmd(MPL115A_PRESSURE_OUT_LSB);
	pressure = (uint16_t) (msb << 8);
	pressure &= (0xFF00 | ((uint16_t) (lsb)));
	pressure = 0x03FF & (pressure >> 6);

	/* calculate the compensated pressure value with the coefficients
	 * and temperature value*/
	tmp1_val = (int32_t) coefficients[MPL115A_C11].value;
	tmp2_val = (int32_t) pressure;
	/*c11x1 = tmp3_val = c11 * pressure*/
	tmp3_val = tmp1_val * tmp2_val;

	tmp1_val = (int32_t) (coefficients[MPL115A_B1].value << 14);
	tmp2_val = (int32_t) tmp3_val;
	tmp3_val = tmp1_val + tmp2_val;
	/*a11 = tmp_help = b1 + c11x1*/
	tmp_help = (int32_t) (tmp3_val >> 14);

	tmp1_val = (int32_t) coefficients[MPL115A_C12].value;
	tmp2_val = (int32_t) temperature;
	/*c12x2 = tmp3_val = c12 * temperature*/
	tmp3_val = tmp1_val * tmp2_val;

	tmp1_val = (int32_t) (tmp_help << 11);
	tmp2_val = (int32_t) tmp3_val;
	tmp3_val = tmp1_val + tmp2_val;
	/*tmp_help = a1 = a11 + c12x2*/
	tmp_help = (int32_t) (tmp3_val >> 11);

	tmp1_val = (int32_t) coefficients[MPL115A_C22].value;
	tmp2_val = (int32_t) temperature;
	/*c22x2 = tmp3_val = c22 * temperature*/
	tmp3_val = tmp1_val * tmp2_val;

	tmp1_val = (int32_t) (coefficients[MPL115A_B2].value << 15);
	tmp2_val = (int32_t) (tmp3_val >> 1);
	tmp3_val = tmp1_val + tmp2_val;

	tmp1_val = (int32_t) tmp_help;
	/*a2 = b2 + c22x2*/
	tmp_help = (int32_t) (tmp3_val >> 16);
	tmp2_val = (int32_t) pressure;
	/*a1x1 = tmp3_val = a1 * pressure*/
	tmp3_val = tmp1_val * tmp2_val;

	tmp1_val = (int32_t) (coefficients[MPL115A_A0].value << 10);
	tmp2_val = (int32_t) tmp3_val;
	tmp3_val = tmp1_val + tmp2_val;

	tmp1_val = (int32_t) tmp_help;
	/*y1 = tmp_help = a0 + a1x1*/
	tmp_help = (int32_t) (tmp3_val >> 10);
	tmp2_val = (int32_t) temperature;
	/*a2x2 = tmp3_val = a2 * temperature*/
	tmp3_val = tmp1_val * tmp2_val;

	tmp1_val = (int32_t) (tmp_help << 10);
	tmp2_val = (int32_t) tmp3_val;
	tmp3_val = tmp1_val + tmp2_val;

	return (int16_t) (tmp3_val >> 13);
}

uint8_t mpl115a_cmd(uint8_t reg) {
	uint8_t data;
	mspi_chip_select(MPL115A_CS);
	mspi_transceive(reg);
	data = mspi_transceive(0x00);
	mspi_chip_release(MPL115A_CS);
	return data;
}
