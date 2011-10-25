/* Copyright (c) 2010, Stephan Rottmann
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
 * \addtogroup Drivers
 * @{
 *
 * \defgroup i2c_driver I2C-Bus Driver
 *
 * <p></p>
 * @{
 *
 */

/**
 * \file
 *		I2C driver definitions
 * \author
 *      Stephan Rottman <rottmann@ibr.cs.tu-bs.de>
 */
#include <avr/io.h>

#ifndef I2CDRV_H_
#define I2CDRV_H_


#define I2C_HIGH_SPEED	 0


#define I2C_START        0x08
#define I2C_REP_START    0x10
#define I2C_MT_SLA_ACK   0x18
#define I2C_MT_DATA_ACK  0x28
#define I2C_MR_SLA_ACK   0x40
#define I2C_MR_DATA_ACK  0x50
#define I2C_MR_DATA_NACK 0x58


void i2c_init(void);
void i2c_stop(void);

int8_t i2c_start(uint8_t addr);
int8_t i2c_rep_start(uint8_t addr);
int8_t i2c_write(uint8_t data);
int8_t i2c_read(uint8_t *data, uint8_t ack);
int8_t i2c_read_ack(uint8_t *data);
int8_t i2c_read_nack(uint8_t *data);


#endif /* I2CDRV_H_ */
