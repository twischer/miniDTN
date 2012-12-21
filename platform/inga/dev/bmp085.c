/*
 * pressure-bmp085.c
 *
 *  Created on: 04.07.2011
 *      Author: root
 */

#include "pressure-bmp085.h"


int8_t bmp085_init(void) {
	uint8_t i = 0;
	i2c_init();
	while (bmp085_read16bit_data(BMP085_AC1_ADDR) == 0x00) {
		_delay_ms(10);
		if (i++ > 10) {
			return -1;
		}
	}
	bmp085_read_calib_data();
	return 0;
}

int32_t bmp085_read_temperature(void) {
	i2c_start(BMP085_DEV_ADDR_W);
	i2c_write(BMP085_CTRL_REG_ADDR);
	i2c_write(BMP085_CTRL_REG_TEMP);
	i2c_stop();
	_delay_ms(5);
	return (int32_t) (bmp085_read16bit_data(BMP085_DATA_REG_N));
}

int32_t bmp085_read_pressure(uint8_t mode) {
	int32_t pressure;
	i2c_start(BMP085_DEV_ADDR_W);
	i2c_write(BMP085_CTRL_REG_ADDR);
	switch (mode) {
	case 0:
		i2c_write(BMP085_CTRL_REG_PRESS_0);
		i2c_stop();
		_delay_ms(5);
		break;
	case 1:
		i2c_write(BMP085_CTRL_REG_PRESS_1);
		i2c_stop();
		_delay_ms(8);
		break;
	case 2:
		i2c_write(BMP085_CTRL_REG_PRESS_2);
		i2c_stop();
		_delay_ms(14);
		break;
	case 3:
		i2c_write(BMP085_CTRL_REG_PRESS_3);
		i2c_stop();
		_delay_ms(26);
		break;
	}
	pressure = bmp085_read16bit_data(BMP085_DATA_REG_N);
	pressure = pressure << 8;
	pressure += bmp085_read8bit_data(BMP085_DATA_REG_X);
	return (pressure >> (8 - mode));
}

int32_t bmp085_read_comp_temperature(void) {
	int32_t ut = 0, compt = 0;

	int32_t x1, x2, b5;

	ut = bmp085_read_temperature();

	x1 = ((int32_t) ut - (int32_t) bmp085_coeff.ac6)
			* (int32_t) bmp085_coeff.ac5 >> 15;
	x2 = ((int32_t) bmp085_coeff.mc << 11) / (x1 + bmp085_coeff.md);
	b5 = x1 + x2;
	compt = (b5 + 8) >> 4;

	return compt;
}

int32_t bmp085_read_comp_pressure(uint8_t mode) {
	int32_t ut = 0, compt = 0;
	int32_t up = 0, compp = 0;

	int32_t x1, x2, b5, b6, x3, b3, p;
	uint32_t b4, b7;

	ut = bmp085_read_temperature();
	up = bmp085_read_pressure(mode);

	x1 = ((int32_t) ut - (int32_t) bmp085_coeff.ac6)
			* (int32_t) bmp085_coeff.ac5 >> 15;
	x2 = ((int32_t) bmp085_coeff.mc << 11) / (x1 + bmp085_coeff.md);
	b5 = x1 + x2;
	compt = (b5 + 8) >> 4;

	b6 = b5 - 4000;
	x1 = (bmp085_coeff.b2 * ((b6 * b6) >> 12)) >> 11;
	x2 = (bmp085_coeff.ac2 * b6) >> 11;
	x3 = x1 + x2;
	b3 = (((((int32_t) bmp085_coeff.ac1) * 4 + x3) << mode) + 2) >> 2;
	x1 = (bmp085_coeff.ac3 * b6) >> 13;
	x2 = (bmp085_coeff.b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (bmp085_coeff.ac4 * (uint32_t) (x3 + 32768)) >> 15;
	b7 = ((uint32_t) (up - b3) * (50000 >> mode));

	if (b7 < 0x80000000) {
		p = (b7 << 1) / b4;
	} else {
		p = (b7 / b4) << 1;
	}

	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	compp = p + ((x1 + x2 + 3791) >> 4);
	return compp;
}

void bmp085_read_calib_data(void) {
	bmp085_coeff.ac1 = bmp085_read16bit_data(BMP085_AC1_ADDR);
	bmp085_coeff.ac2 = bmp085_read16bit_data(BMP085_AC2_ADDR);
	bmp085_coeff.ac3 = bmp085_read16bit_data(BMP085_AC3_ADDR);
	bmp085_coeff.ac4 = bmp085_read16bit_data(BMP085_AC4_ADDR);
	bmp085_coeff.ac5 = bmp085_read16bit_data(BMP085_AC5_ADDR);
	bmp085_coeff.ac6 = bmp085_read16bit_data(BMP085_AC6_ADDR);
	bmp085_coeff.b1 = bmp085_read16bit_data(BMP085_B1_ADDR);
	bmp085_coeff.b2 = bmp085_read16bit_data(BMP085_B2_ADDR);
	bmp085_coeff.mb = bmp085_read16bit_data(BMP085_MB_ADDR);
	bmp085_coeff.mc = bmp085_read16bit_data(BMP085_MC_ADDR);
	bmp085_coeff.md = bmp085_read16bit_data(BMP085_MD_ADDR);
}

uint16_t bmp085_read16bit_data(uint8_t addr) {
	uint8_t msb = 0, lsb = 0;
	i2c_start(BMP085_DEV_ADDR_W);
	i2c_write(addr);
	i2c_rep_start(BMP085_DEV_ADDR_R);
	i2c_read_ack(&msb);
	i2c_read_nack(&lsb);
	i2c_stop();
	return (uint16_t) ((msb << 8) | lsb);
}

uint8_t bmp085_read8bit_data(uint8_t addr) {
	uint8_t lsb = 0;
	i2c_start(BMP085_DEV_ADDR_W);
	i2c_write(addr);
	i2c_rep_start(BMP085_DEV_ADDR_R);
	i2c_read_nack(&lsb);
	i2c_stop();
	return lsb;
}
