#ifndef __XMEGA_SPI_H__
#define __XMEGA_SPI_H__

#include <avr/io.h>
#include <stdint.h>

/*
	CLK2X			|      0 |      1 |
	-------------------------------------------------
	SPI_PRESCALER_DIV4_gc	|   /4   |    /2  |
	SPI_PRESCALER_DIV16_gc	|   /16  |    /8  |
	SPI_PRESCALER_DIV64_gc	|   /64  |    /32 |	
	SPI_PRESCALER_DIV128_gc	|   /128 |    /64 |
*/

#ifndef SPI_PRESCALER_DIV2_gc
	#define SPI_PRESCALER_DIV2_gc (SPI_PRESCALER_DIV4_gc | SPI_CLK2X_bm)
#endif

#ifndef SPI_PRESCALER_DIV8_gc
	#define SPI_PRESCALER_DIV8_gc (SPI_PRESCALER_DIV16_gc | SPI_CLK2X_bm)
#endif

#ifndef SPI_PRESCALER_DIV32_gc
	#define SPI_PRESCALER_DIV32_gc (SPI_PRESCALER_DIV64_gc | SPI_CLK2X_bm)
#endif


#endif // #ifndef __XMEGA_SPI_H__
