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
 * \addtogroup adxl345_interface
 * @{
 */

/**
 * \file
 *      ADXL345 Accelerometer interface implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */
#include "acc-adxl345.h"

int8_t adxl345_init(void) {
	uint8_t i = 0;
	mspi_chip_release(ADXL345_CS);
	mspi_init(ADXL345_CS, MSPI_MODE_3, MSPI_BAUD_MAX);

	adxl345_write(ADXL345_DATA_FORMAT_REG, ADXL345_DATA_FORMAT_DATA);
	adxl345_write(ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL_DATA);
	adxl345_write(ADXL345_BW_RATE_REG, ADXL345_BW_RATE_DATA);

	while (adxl345_read(ADXL345_DEVICE_ID_REG) != ADXL345_DEVICE_ID_DATA) {
		_delay_ms(10);
		if (i++ > 250) {
			return -1;
		}
	}
	return 0;

}

uint16_t adxl345_get_x_acceleration(void) {
	uint8_t byteLow;
	uint8_t byteHigh;
	byteLow = adxl345_read(ADXL345_OUTX_LOW_REG);
	byteHigh = adxl345_read(ADXL345_OUTX_HIGH_REG);

	return ((byteHigh << 8) + byteLow);
}

uint16_t adxl345_get_y_acceleration(void) {
	uint8_t byteLow;
	uint8_t byteHigh;
	byteLow = adxl345_read(ADXL345_OUTY_LOW_REG);
	byteHigh = adxl345_read(ADXL345_OUTY_HIGH_REG);
	return (byteHigh << 8) + byteLow;
}

uint16_t adxl345_get_z_acceleration(void) {
	uint8_t byteLow;
	uint8_t byteHigh;
	byteLow = adxl345_read(ADXL345_OUTZ_LOW_REG);
	byteHigh = adxl345_read(ADXL345_OUTZ_HIGH_REG);
	return (byteHigh << 8) + byteLow;
}

acc_data_t adxl345_get_acceleration(void) {
	acc_data_t adxl345_data;
	adxl345_data.acc_x_value = adxl345_get_x_acceleration();
	adxl345_data.acc_y_value = adxl345_get_y_acceleration();
	adxl345_data.acc_z_value = adxl345_get_z_acceleration();
	return adxl345_data;
}

void adxl345_write(uint8_t reg, uint8_t data) {
	mspi_chip_select(ADXL345_CS);
	reg &= 0x7F;
	mspi_transceive(reg);
	mspi_transceive(data);
	mspi_chip_release(ADXL345_CS);
}

uint8_t adxl345_read(uint8_t reg) {
	uint8_t data;
	mspi_chip_select(ADXL345_CS);
	reg |= 0x80;
	mspi_transceive(reg);
	data = mspi_transceive(MSPI_DUMMY_BYTE);
	mspi_chip_release(ADXL345_CS);
	return data;
}

