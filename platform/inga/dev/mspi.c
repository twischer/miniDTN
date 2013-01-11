/*
 * Copyright (c) 2012, TU Braunschweig.
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
 *      MSPI driver implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_bus_driver
 * @{
 */

/**
 * \addtogroup mspi_driver
 * @{
 */

#include "mspi.h"


/* global variable for the selected USART port*/
uint8_t mspi_uart_port = MSPI_USART1;
void
mspi_init(uint8_t cs, uint8_t mode, uint16_t baud) {
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
/*----------------------------------------------------------------------------*/
void
mspi_chip_select(uint8_t cs) {
#if MSPI_BUS_MANAGER
  if (spi_current_config != spi_bus_config[cs].checksum) {
    /*new mspi configuration is needed by this spi device*/
    change_spi_mode(spi_bus_config[cs]);
    spi_current_config = spi_bus_config[cs].checksum;
  }
#endif
  /*chip select*/
  MSPI_CS_PORT |= cs_bcd[cs];
}
/*----------------------------------------------------------------------------*/
void
mspi_chip_release(uint8_t cs) {
  /*chip deselect*/
  MSPI_CS_PORT &= ~((1 << MSPI_CS_PIN_0) | (1 << MSPI_CS_PIN_1) | (1 << MSPI_CS_PIN_2));
}
/*----------------------------------------------------------------------------*/
uint8_t
mspi_transceive(uint8_t data) {
  uint8_t receive_data;

  /*wait while transmit buffer is empty*/
  while (!(*(usart_ports[mspi_uart_port].UCSRnA) & (1 << UDRE0)));
  *(usart_ports[mspi_uart_port].UDRn) = data;

  /*wait to readout the MSPI data register*/
  while (!(*(usart_ports[mspi_uart_port].UCSRnA) & (1 << RXC0)));
  receive_data = *(usart_ports[mspi_uart_port].UDRn);

  return receive_data;
}
/*----------------------------------------------------------------------------*/
void
mspi_deinit(void) {
  *(usart_ports[mspi_uart_port].UCSRnA) = MSPI_DISABLE;
  *(usart_ports[mspi_uart_port].UCSRnB) = MSPI_DISABLE;
  *(usart_ports[mspi_uart_port].UCSRnC) = MSPI_DISABLE;
}
/*----------------------------------------------------------------------------*/

#if MSPI_BUS_MANAGER
void
add_to_spi_mgr(uint8_t cs, uint8_t mode, uint16_t baud) {
  spi_bus_config[cs].dev_mode = mode;
  spi_bus_config[cs].dev_baud = baud;
  spi_bus_config[cs].checksum = mode + baud;
}
/*----------------------------------------------------------------------------*/
void
change_spi_mode(spi_dev new_config) {
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
