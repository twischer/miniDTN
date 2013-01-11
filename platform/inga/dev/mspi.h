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
 *		MSPI driver definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_bus_driver
 * @{
 */

/**
 * \defgroup mspi_driver Master SPI Bus Driver (MSPI)
 *
 * <p>There are a lot of possibilities to realize a Serial Peripheral Interface (SPI) with the
 * ATmega1284p, but the hardware SPI is used by the transceiver chip AT86rf230. In combination
 * with contiki it is not possible to build up an SPI Bus with this hardware SPI.
 * A software SPI would be a solution, but the ATmega1284p provides another opportunity. The MSPI
 * (Maser SPI) is an alternative function of the USART. There are two independent hardware USART,
 *  so it is possible to use one for the default debug RS232 interface, while the other one is
 *  free to be used as an MSPI. The only difference between an normal SPI and the MSPI is, that
 *  MSPI only supports slaves on the bus.</p>
 * @{
 */

#ifndef MSPIDRV_H_
#define MSPIDRV_H_

#include <avr/io.h>
/*!
 * Enable or disable the MSPI-Bus Manager.
 *
 * \note The...
 */
#define MSPI_BUS_MANAGER	1
#if MSPI_BUS_MANAGER
#include "mspi-mgr.h"
#endif

#define MSPI_USART0			0
#define MSPI_USART1			1
/*\cond*/
#define MSPI_ENABLE			(0xC0)
#define MSPI_DISABLE		(0x06)
#define MSPI_CS_DISABLE		0
#define MSPI_DUMMY_BYTE		0xFF
/*\endcond*/


#define MSPI_CS_PORT		PORTA
#define MSPI_CS_PORT_DDR	DDRA
#define MSPI_CS_PIN_0		PORTA5
#define MSPI_CS_PIN_1		PORTA6
#define MSPI_CS_PIN_2		PORTA7

/*!
 * This array holds the BCD for the 8 possible SPI devices, because the few amount
 * of port-pins made it necessary to use a BCD-Decimal-Encoder. The first item
 * (chip_select[0]) disables chip select. The other (chip_select[1] ...
 * chip_select[7]) holds the MSPI_SC_PORT information for the specific SPI device.
 *
 * \note This is a special solution for the raven based ibr sensor node. If you want
 * to use this driver for other applications, feel free to change the chip select
 * management
 */
static uint8_t cs_bcd[8] = {
		/*Chip Select Disable*/
		(0xFF & (0 << MSPI_CS_PIN_0) & (0 << MSPI_CS_PIN_1) & (0 << MSPI_CS_PIN_2)),
		/*Chip Select Enable for specific SPI device (1...7)*/
		(0x00 | (1 << MSPI_CS_PIN_0) | (0 << MSPI_CS_PIN_1) | (0 << MSPI_CS_PIN_2)), //1
		(0x00 | (0 << MSPI_CS_PIN_0) | (1 << MSPI_CS_PIN_1) | (0 << MSPI_CS_PIN_2)), //2
		(0x00 | (1 << MSPI_CS_PIN_0) | (1 << MSPI_CS_PIN_1) | (0 << MSPI_CS_PIN_2)), //3
		(0x00 | (0 << MSPI_CS_PIN_0) | (0 << MSPI_CS_PIN_1) | (1 << MSPI_CS_PIN_2)), //4
		(0x00 | (1 << MSPI_CS_PIN_0) | (0 << MSPI_CS_PIN_1) | (1 << MSPI_CS_PIN_2)), //5
		(0x00 | (0 << MSPI_CS_PIN_0) | (1 << MSPI_CS_PIN_1) | (1 << MSPI_CS_PIN_2)), //6
		(0x00 | (1 << MSPI_CS_PIN_0) | (1 << MSPI_CS_PIN_1) | (1 << MSPI_CS_PIN_2))  //7

};

/********************************************************************
 * MSPI mode:
 ********************************************************************/

/*!
 * MSPI Mode 0
 * \note <ul>
 * 		 <li> Leading Edge: Sample (Rising Edge)
 * 		 <li> Trailing Edge: Setup (Falling Edge)
 * 		 </ul>
 */
#define MSPI_MODE_0			(0x00)
/*!
 * MSPI Mode 1
 * \note <ul>
 * 		 <li> Leading Edge: Setup (Rising Edge)
 * 		 <li> Trailing Edge: Sample (Falling Edge)
 * 		 </ul>
 */
#define MSPI_MODE_1			(0x02)
/*!
 * MSPI Mode 2
 * \note <ul>
 * 		 <li> Leading Edge: Sample (Falling Edge)
 * 		 <li> Trailing Edge: Setup (Rising Edge)
 * 		 </ul>
 */
#define MSPI_MODE_2			(0x01)
/*!
 * MSPI Mode 3
 * \note <ul>
 * 		 <li> Leading Edge: Setup (Falling Edge)
 * 		 <li> Trailing Edge: Sample (Rising Edge)
 * 		 </ul>
 */
#define MSPI_MODE_3			(0x03)

/********************************************************************
 * MSPI BAUD Rate:
 ********************************************************************/

/*!
 * Maximum MSPI Baud Rate [bps]
 * \note The maximum baud rate is f_osc/2
 */
#define MSPI_BAUD_MAX		(0x00)
/*!
 * 2Mbps MSPI Baud Rate
 * \note Assumption: f_osc = 8 MHz
 */
#define MSPI_BAUD_2MBPS		(0x01)
/*!
 * 1Mbps MSPI Baud Rate
 * \note Assumption: f_osc = 8 MHz
 */
#define MSPI_BAUD_1MBPS		(0x03)


typedef struct {
 /*!
  * MSPI Baud Rate Register (USART0/USART1)
  */
  volatile uint16_t * UBRRn;
 /*!
  * MSPI Serial Clock Pin Data Direction Register (USART0/USART1)
  */
  volatile uint8_t * XCKn_DDR; //Uart0 = DDRB //Uart1 = DDRD
 /*!
  * MSPI Serial Clock Pin (USART0/USART1)
  */
  volatile uint8_t XCKn; //Uart0 = PORTB0 //Uart1 = PORTD4
 /*!
  * MSPIM Control and Status Register A (USART0/USART1)
  */
  volatile uint8_t * UCSRnA;
 /*!
  * MSPIM Control and Status Register B (USART0/USART1)
  */
  volatile uint8_t * UCSRnB;
 /*!
  * MSPIM Control and Status Register C (USART0/USART1)
  */
  volatile uint8_t * UCSRnC;
  /*!
   * MSPI I/O Data Register (USART0/USART1)
   */
  volatile uint8_t * UDRn;
} usart_t;

static usart_t usart_ports[2] = {
  {   // MSPI UART0
    &UBRR0,
    &DDRB,
     PORTB0,
    &UCSR0A,
    &UCSR0B,
    &UCSR0C,
    &UDR0
  },

  {  // MSPI UART1
	&UBRR1,
	&DDRD,
	 PORTD4,
	&UCSR1A,
	&UCSR1B,
	&UCSR1C,
	&UDR1
  }
};

/**
 * \brief Initialize the selected USART in the MSPI mode
 *
 * \param mode Select the (M)SPI mode (MSPI_MODE_0 ...
 * 	      MSPI_MODE_3)
 * \param baud The MSPI BAUD rate. Sometimes it is necessary
 *        to reduce the SCK. Use MSPI_BAUD_MAX in common case.
 *
 */
void mspi_init(uint8_t cs, uint8_t mode, uint16_t baud);

/**
 * \brief This function can be use either to transmit or receive
 *        data via spi.
 *
 * \param data <ul>
 * 			   <li>When use this function to transmit: Data byte, which
 * 			   has to be transmit.
 * 			   <li>When use this function to receive: Data value don't
 * 			   care. (Dummy Byte, e.g. 0xFF)
 * 			   </ul>
 * \return     <li>When use this function to transmit: The return value
 * 			   doesn't care
 *             <li>When use this function to receive: Received data
 *             from the spi slave
 *             </ul>
 *
 * \note The various devices along the SPI Bus are separated by the
 *  Chip Select (cs). The assignment of the SPI-devices depends on
 *  the hardware wiring. The correct chip select an spi configuration
 *  will be done by the spi-bus.manager. Otherwise please ensure,
 *  which Chip Select and spi bus configuration belong to which
 *  SPI device.
 */
uint8_t mspi_transceive(uint8_t data);

/**
 * \brief This function enables the chip select by setting the
 *        needed I/O pins (BCD-Code)
 *
 * \param cs   Chip Select: Device ID
 */
void mspi_chip_select(uint8_t cs);

/**
 * \brief This function disables the chip select
 *
 * \param cs   Chip Select: Device ID
 */
void mspi_chip_release(uint8_t cs);

/**
 * \brief This function will set all MSPI registers to their
 *        default values.
 */
void mspi_deinit(void);

/** @} */ // mspi_driver
/** @} */ // inga_bus_driver

#endif /* MSPIDRV_H_ */
