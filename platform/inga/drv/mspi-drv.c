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
 * \addtogroup Drivers
 * @{
 *
 * \addtogroup mspi_driver
 * @{
 */

/**
 * \file
 *      MSPI driver implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

#include "mspi-drv.h"


/* global variable for the selected USART port*/
uint8_t mspi_uart_port = MSPI_USART1;

void mspi_init(uint8_t cs, uint8_t mode, uint16_t baud){
	/*initialize the spi port pins, setting pins output*/
	MSPI_CS_PORT_DDR |= (1 << MSPI_CS_PIN_0) | (1 << MSPI_CS_PIN_1) | (1 << MSPI_CS_PIN_2);
	MSPI_CS_PORT &= ~((1 << MSPI_CS_PIN_0) | (1 << MSPI_CS_PIN_1) | (1 << MSPI_CS_PIN_2));

#if MSPI_BUS_MANAGER
	add_to_spi_mgr(cs, mode, baud);
#else
	/*initialize MSPI*/
	*(usart_ports[mspi_uart_port].UBRRn) = 0;
	/*setting clock pin as output*/
	*(usart_ports[mspi_uart_port].XCKn_DDR) |= (1 << usart_ports[mspi_uart_port].XCKn);
	/*enable MSPI and set spi mode*/
	*(usart_ports[mspi_uart_port].UCSRnC) = ((MSPI_ENABLE) | mode);
	/*initialize RX and TX*/
    *(usart_ports[mspi_uart_port].UCSRnB) = ((1 << RXEN0) | (1 << TXEN0));
    *(usart_ports[mspi_uart_port].UBRRn) = baud;
#endif
}


void mspi_chip_select(uint8_t cs){
#if MSPI_BUS_MANAGER
	if(spi_current_config != spi_bus_config[cs].checksum){
		/*new mspi configuration is needed by this spi device*/
		change_spi_mode(spi_bus_config[cs]);
		spi_current_config = spi_bus_config[cs].checksum;
	}
#endif
	/*chip select*/
	MSPI_CS_PORT |= cs_bcd[cs];
}

void mspi_chip_release(uint8_t cs){
	/*chip deselect*/
	MSPI_CS_PORT &= ~((1 << MSPI_CS_PIN_0) | (1 << MSPI_CS_PIN_1) | (1 << MSPI_CS_PIN_2));
}


uint8_t mspi_transceive(uint8_t data){
	uint8_t receive_data;

	/*wait while transmit buffer is empty*/
	while ( !(*(usart_ports[mspi_uart_port].UCSRnA) & (1<<UDRE0)));
		    *(usart_ports[mspi_uart_port].UDRn) = data;

	/*wait to readout the MSPI data register*/
	while ( !(*(usart_ports[mspi_uart_port].UCSRnA) & (1<<RXC0)) );
			receive_data = *(usart_ports[mspi_uart_port].UDRn);

	return receive_data;
}

void mspi_deinit(void){
	*(usart_ports[mspi_uart_port].UCSRnA) = MSPI_DISABLE;
	*(usart_ports[mspi_uart_port].UCSRnB) = MSPI_DISABLE;
	*(usart_ports[mspi_uart_port].UCSRnC) = MSPI_DISABLE;
}

#if MSPI_BUS_MANAGER
void add_to_spi_mgr(uint8_t cs, uint8_t mode, uint16_t baud){
	spi_bus_config[cs].dev_mode = mode;
	spi_bus_config[cs].dev_baud = baud;
	spi_bus_config[cs].checksum = mode + baud;
}

void change_spi_mode(spi_dev new_config){
	/*initialize MSPI*/
	*(usart_ports[mspi_uart_port].UBRRn) = 0;
	/*setting clock pin as output*/
	*(usart_ports[mspi_uart_port].XCKn_DDR) |= (1 << usart_ports[mspi_uart_port].XCKn);
	/*enable MSPI and set spi mode*/
	*(usart_ports[mspi_uart_port].UCSRnC) = ((MSPI_ENABLE) | new_config.dev_mode);
	/*initialize RX and TX*/
	*(usart_ports[mspi_uart_port].UCSRnB) = ((1 << RXEN0) | (1 << TXEN0));
	*(usart_ports[mspi_uart_port].UBRRn) = new_config.dev_baud;
}
#endif
