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
 * \addtogroup i2c_driver
 * @{
 */

/**
 * \file
 *      I2C driver implementation
 * \author
 *      Stephan Rottman <rottmann@ibr.cs.tu-bs.de>
 *      modified by Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

#include "i2c-drv.h"

#ifndef PRR
#define PRR PRR0
#endif

void i2c_init(void)
{
	TWSR &= ~((1<<TWPS1) | (1<<TWPS0));
#if I2C_HIGH_SPEED
	TWBR = 2;	//400KHz
#else
	TWBR = 32;	//100KHz
#endif

}


int8_t _i2c_start(uint8_t addr, uint8_t rep)
{
	PRR &= ~(1<<PRTWI);
	i2c_init();
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	if((TWSR&0xF8) != ((!rep)?I2C_START:I2C_REP_START))
		return -1;

	TWDR = addr;
	TWCR = (1<<TWINT) | (1<<TWEN);

	while(!(TWCR & (1<<TWINT)));
	if(!(((TWSR&0xF8) == I2C_MT_SLA_ACK) || ((TWSR&0xF8) == I2C_MR_SLA_ACK)))
		return -2;

	return 0;
}

int8_t i2c_start(uint8_t addr)
{
	return _i2c_start(addr, 0);
}

int8_t i2c_rep_start(uint8_t addr)
{
	return _i2c_start(addr, 1);
}

int8_t i2c_write(uint8_t data)
{
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);

	while(!(TWCR & (1<<TWINT)));
	if((TWSR&0xF8) != I2C_MT_DATA_ACK)
		return -1;

	return 0;
}

int8_t i2c_read(uint8_t *data, uint8_t ack)
{
	uint16_t i = 0;
	TWCR = (1<<TWINT)|(1<<TWEN)| (ack?(1<<TWEA):0);
	while(!(TWCR & (1<<TWINT))){
		if(i++ > 800){
			return -1;
		}
	}
	if((TWSR&0xF8) != (ack?I2C_MR_DATA_ACK:I2C_MR_DATA_NACK))
		return -1;
	*data = TWDR;
	return 0;
}

int8_t i2c_read_ack(uint8_t *data)
{
	return i2c_read(data, 1);
}

int8_t i2c_read_nack(uint8_t *data)
{
	return i2c_read(data, 0);
}

void i2c_stop(void)
{

	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	while(TWCR & (1<<TWSTO));

	TWCR &= ~(TWEN);

	PRR |= (1<<PRTWI);
	DDRC &= ~( (1<<PC0) | (1<<PC1) );
	PORTC |= ((1<<PC0) | (1<<PC1));

}
