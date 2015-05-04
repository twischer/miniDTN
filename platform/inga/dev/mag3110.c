/*
 * Copyright (c) 2014, TU Braunschweig.
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
 *      Freescale MAG3110 3-axis Magnetometer interface implementation
 * \author
 *      Yannic Schröder <yschroed@ibr.cs.tu-bs.de>
 *
 */

/**
 * \addtogroup mag3110_interface
 * @{
 */

#include "mag3110.h"

/*----------------------------------------------------------------------------*/
uint8_t
mag3110_available(void)
{
	i2c_init();	
	int8_t code = i2c_start(MAG3110_ADDRESS << 1);
	i2c_stop();

	if (code == 0) {
		return 1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
mag3110_init(void)
{
	if (!mag3110_available()) {
		return -1;
	}

	uint8_t ctrl1_default = (MAG3110_DR_1280 << MAG3110_DR_OFFSET)	// ADC Sample Rate 1280 Hz
			| (MAG3110_OS_16 << MAG3110_OS_OFFSET)					// 16x Oversampling
			| (MAG3110_FR_OFF << MAG3110_FR_BIT)					// full 16-bit readings
			| (MAG3110_TM_OFF << MAG3110_TM_BIT)					// trigger off
			| (MAG3110_AC_ACTIVE << MAG3110_AC_BIT);				// free running mode
	uint8_t ctrl2_default = (MAG3110_AUTO_MRST_ENABLED << MAG3110_AUTO_MRST_EN_BIT)	// automatic magnetic resets
			| (MAG3110_RAW_NORMAL << MAG3110_RAW_BIT);				// user offsets are applied

	i2c_init();
	i2c_start(MAG3110_DEV_ADDR_W);
	i2c_write(MAG3110_CTRL_REG1);
	i2c_write(ctrl1_default);
	i2c_write(ctrl2_default);
	i2c_stop();

	return 0;
}
/*----------------------------------------------------------------------------*/
void
mag3110_deinit(void) {
	// set magnetometer to low power state
	mag3110_deactivate();
}
/*----------------------------------------------------------------------------*/
void
mag3110_activate(void) {
	uint8_t ctrl1 = mag3110_read8bit(MAG3110_CTRL_REG1);
	ctrl1 |= (MAG3110_AC_ACTIVE << MAG3110_AC_BIT);
	mag3110_write8bit(MAG3110_CTRL_REG1, ctrl1);
}
/*----------------------------------------------------------------------------*/
void
mag3110_deactivate(void) {
	uint8_t ctrl1 = mag3110_read8bit(MAG3110_CTRL_REG1);
	ctrl1 &= ~(MAG3110_AC_ACTIVE << MAG3110_AC_BIT);
	mag3110_write8bit(MAG3110_CTRL_REG1, ctrl1);
}
/*----------------------------------------------------------------------------*/
mag_data_t
mag3110_get(void)
{
	mag_data_t mag3110_data;
	uint8_t data_low;
	uint8_t data_high;
	i2c_start(MAG3110_DEV_ADDR_W);
	i2c_write(MAG3110_OUT_X_MSB); // first channel in fast mode
	i2c_rep_start(MAG3110_DEV_ADDR_R);
	i2c_read_ack(&data_high);
	i2c_read_ack(&data_low);
	mag3110_data.x = ( (data_high << 8) | data_low);
	i2c_read_ack(&data_high);
	i2c_read_ack(&data_low);
	mag3110_data.y = ( (data_high << 8) | data_low);
	i2c_read_ack(&data_high);
	i2c_read_ack(&data_low);
	mag3110_data.z = ( (data_high << 8) | data_low);
	i2c_read_nack(&data_high);
	i2c_stop();
	return mag3110_data;
}
/*----------------------------------------------------------------------------*/
int16_t
mag3110_get_x(void) {
	return mag3110_read16bit(MAG3110_OUT_X_MSB);
}
/*----------------------------------------------------------------------------*/
int16_t
mag3110_get_y(void) {
	return mag3110_read16bit(MAG3110_OUT_Y_MSB);
}
/*----------------------------------------------------------------------------*/
int16_t
mag3110_get_z(void) {
	return mag3110_read16bit(MAG3110_OUT_Z_MSB);
}
/*----------------------------------------------------------------------------*/
int8_t
mag3110_get_temp(void) {
	return (int8_t)mag3110_read8bit(MAG3110_DIE_TEMP);
}
/*----------------------------------------------------------------------------*/
uint8_t
mag3110_read8bit(uint8_t addr)
{
	uint8_t lsb = 0;
	i2c_start(MAG3110_DEV_ADDR_W);
	i2c_write(addr);
	i2c_rep_start(MAG3110_DEV_ADDR_R);
	i2c_read_nack(&lsb);
	i2c_stop();
	return lsb;
}
/*----------------------------------------------------------------------------*/
int16_t
mag3110_read16bit(uint8_t addr)
{
	uint8_t lsb = 0, msb = 0;
	i2c_start(MAG3110_DEV_ADDR_W);
	i2c_write(addr);
	i2c_rep_start(MAG3110_DEV_ADDR_R);
	i2c_read_ack(&msb);
	i2c_read_nack(&lsb);
	i2c_stop();
	return (int16_t) ((msb << 8) + lsb);
}
/*----------------------------------------------------------------------------*/
void
mag3110_write8bit(uint8_t addr, uint8_t data)
{
	i2c_start(MAG3110_DEV_ADDR_W);
	i2c_write(addr);
	i2c_write(data);
	i2c_stop();
}
/*----------------------------------------------------------------------------*/
void
mag3110_write16bit(uint8_t addr, int16_t data)
{
	uint8_t lsb = (data & 0xFF);
	uint8_t msb = (data >> 8);
	i2c_start(MAG3110_DEV_ADDR_W);
	i2c_write(addr);
	i2c_write(msb);
	i2c_write(lsb);
	i2c_stop();
}

