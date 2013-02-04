/*
 * Copyright (c) 2006, Technical University of Munich
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
 *
 * This file is part of the Contiki operating system.
 *
 * @(#)$$
 */

/**
 * \file
 *         AVR specific definitions for the rs232 port.
 *
 * \author
 *         Simon Barner <barner@in.tum.de
 */

#ifndef __RS232_ATXMEGA256A3__
#define __RS232_ATXMEGA256A3__
/******************************************************************************/
/***   Includes                                                               */
/******************************************************************************/
#include <avr/io.h>

/******************************************************************************/
/***   RS232 ports                                                            */
/******************************************************************************/

#define RS232_0 USARTE0
#define RS232_1 USARTF0

#define RS232_USARTE0 0
#define RS232_USARTF0 1

#define RS232_PORT_0 0
#define RS232_PORT_1 1

#define RS232_COUNT 2

/******************************************************************************/
/***   Baud rates                                                             */
/******************************************************************************/

#if (F_CPU == 48000000UL)
	#define USART_BAUD_2400 0x04E1
	#define USART_BAUD_4800 0x0270
	#define USART_BAUD_9600 0xD9BC
	#define USART_BAUD_14400 0xCCF5
	#define USART_BAUD_19200 0xC9B4
	#define USART_BAUD_28800 0xBCE5
	#define USART_BAUD_38400 0xB9A4
	#define USART_BAUD_57600 0xACC5
	#define USART_BAUD_76800 0xA984
	#define USART_BAUD_115200 0x9C85
	#define USART_BAUD_230400 0x9603
	#define USART_BAUD_250000 0x000B
	#define USART_BAUD_500000 0x0005
	#define USART_BAUD_1000000 0x0002
#elif (F_CPU == 32000000UL)
/* Single speed operation (U2X = 0)*/
	#define USART_BAUD_2400 0x0340
	#define USART_BAUD_4800 0xDCFD
	#define USART_BAUD_9600 0xCCF5
	#define USART_BAUD_14400 0xC89E
	#define USART_BAUD_19200 0xBCE5
	#define USART_BAUD_28800 0xB88E
	#define USART_BAUD_38400 0xACC5
	#define USART_BAUD_57600 0xA86E
	#define USART_BAUD_76800 0x9C85
	#define USART_BAUD_115200 0x982E
	#define USART_BAUD_230400 0x93D7
	#define USART_BAUD_250000 0x0007
	#define USART_BAUD_500000 0x0003
	#define USART_BAUD_1000000 0x0001
#elif (F_CPU == 18000000UL)
	#define USART_BAUD_2400 0xDE9E
	#define USART_BAUD_4800 0xCE96
	#define USART_BAUD_9600 0xBE86
	#define USART_BAUD_14400 0xB9A4
	#define USART_BAUD_19200 0xAE66
	#define USART_BAUD_28800 0xA984
	#define USART_BAUD_38400 0x9E26
	#define USART_BAUD_57600 0x9944
	#define USART_BAUD_76800 0x96D3
	#define USART_BAUD_115200 0x9462
	#define USART_BAUD_230400 0x91F1
	#define USART_BAUD_250000 0x91C0
	#define USART_BAUD_500000 0x90A0
	#define USART_BAUD_1000000 0x9010
#elif (F_CPU == 20000000UL)
	#define USART_BAUD_2400 0xE81F
	#define USART_BAUD_4800 0xD81B
	#define USART_BAUD_9600 0xC813
	#define USART_BAUD_14400 0xBABA
	#define USART_BAUD_19200 0xB803
	#define USART_BAUD_28800 0xAA9A
	#define USART_BAUD_38400 0x9FC7
	#define USART_BAUD_57600 0x9A5A
	#define USART_BAUD_76800 0x97A3
	#define USART_BAUD_115200 0x94ED
	#define USART_BAUD_230400 0x9236
	#define USART_BAUD_250000 0x0004
	#define USART_BAUD_500000 0x90C0
	#define USART_BAUD_1000000 0x9020
#elif (F_CPU == 16000000UL)
/* Single speed operation (U2X = 0)*/
	#define USART_BAUD_2400 0xDCFD
	#define USART_BAUD_4800 0xCCF5
	#define USART_BAUD_9600 0xBCE5
	#define USART_BAUD_14400 0xB88E
	#define USART_BAUD_19200 0xACC5
	#define USART_BAUD_28800 0xA86E
	#define USART_BAUD_38400 0x9C85
	#define USART_BAUD_57600 0x982E
	#define USART_BAUD_76800 0x9603
	#define USART_BAUD_115200 0x93D7
	#define USART_BAUD_230400 0x91AC
	#define USART_BAUD_250000 0x0003
	#define USART_BAUD_500000 0x0001
	#define USART_BAUD_1000000 0x0001
#elif (F_CPU == 12000000UL)
	#define USART_BAUD_2400 0xD9BC
	#define USART_BAUD_4800 0xC9B4
	#define USART_BAUD_9600 0xB9A4
	#define USART_BAUD_14400 0xACC5
	#define USART_BAUD_19200 0xA984
	#define USART_BAUD_28800 0x9C85
	#define USART_BAUD_38400 0x9944
	#define USART_BAUD_57600 0x9603
	#define USART_BAUD_76800 0x9462
	#define USART_BAUD_115200 0x92C1
	#define USART_BAUD_230400 0x9121
	#define USART_BAUD_250000 0x0002
	#define USART_BAUD_500000 0x9040
	#define USART_BAUD_1000000 0x9040
#elif (F_CPU == 8000000UL)
/* Single speed operation (U2X = 0)*/
	#define USART_BAUD_2400 0xCCF5
	#define USART_BAUD_4800 0xBCE5
	#define USART_BAUD_9600 0xACC5
	#define USART_BAUD_14400 0xA86E
	#define USART_BAUD_19200 0x9C85
	#define USART_BAUD_28800 0x982E
	#define USART_BAUD_38400 0x9603
	#define USART_BAUD_57600 0x93D7
	#define USART_BAUD_76800 0x92C1
	#define USART_BAUD_115200 0x91AC
	#define USART_BAUD_230400 0x9096
	#define USART_BAUD_250000 0x0001
	#define USART_BAUD_500000 0x0001
	#define USART_BAUD_1000000 0x0001
#elif (F_CPU == 6000000UL)
	#define USART_BAUD_2400 0xC9B4
	#define USART_BAUD_4800 0xB9A4
	#define USART_BAUD_9600 0xA984
	#define USART_BAUD_14400 0x9C85
	#define USART_BAUD_19200 0x9944
	#define USART_BAUD_28800 0x9603
	#define USART_BAUD_38400 0x9462
	#define USART_BAUD_57600 0x92C1
	#define USART_BAUD_76800 0x91F1
	#define USART_BAUD_115200 0x9121
	#define USART_BAUD_230400 0x9050
	#define USART_BAUD_250000 0x9040
	#define USART_BAUD_500000 0x9040
	#define USART_BAUD_1000000 0x9040
#elif (F_CPU == 4000000UL)
	#define USART_BAUD_2400 0xBCE5
	#define USART_BAUD_4800 0xACC5
	#define USART_BAUD_9600 0x9C85
	#define USART_BAUD_14400 0x982E
	#define USART_BAUD_19200 0x9603
	#define USART_BAUD_28800 0x93D7
	#define USART_BAUD_38400 0x92C1
	#define USART_BAUD_57600 0x91AC
	#define USART_BAUD_76800 0x9121
	#define USART_BAUD_115200 0x9096
	#define USART_BAUD_230400 0x900B
	#define USART_BAUD_250000 0x900B
	#define USART_BAUD_500000 0x900B
	#define USART_BAUD_1000000 0x900B
#elif (F_CPU == 2000000UL)
	#define USART_BAUD_2400 0xACC5
	#define USART_BAUD_4800 0x9C85
	#define USART_BAUD_9600 0x9603
	#define USART_BAUD_14400 0x93D7
	#define USART_BAUD_19200 0x92C1
	#define USART_BAUD_28800 0x91AC
	#define USART_BAUD_38400 0x9121
	#define USART_BAUD_57600 0x9096
	#define USART_BAUD_76800 0x9050
	#define USART_BAUD_115200 0x900B
	#define USART_BAUD_230400 0x900B
	#define USART_BAUD_250000 0x900B
	#define USART_BAUD_500000 0x900B
	#define USART_BAUD_1000000 0x900B
#else
	#error "Please define the baud rates for your CPU clock or set the rate in contiki-conf.h"
#endif


/******************************************************************************/
/***   Interrupt settings                                                     */
/******************************************************************************/

#define USART_INTERRUPT_RX_COMPLETE USART_RXCIF_bm
#define USART_INTERRUPT_TX_COMPLETE USART_TXCIF_bm
#define USART_INTERRUPT_DATA_REG_EMPTY USART_DREIF_bm


/******************************************************************************/
/***   Receiver / transmitter                                                 */
/******************************************************************************/

#define USART_RECEIVER_ENABLE USART_RXEN_bm
#define USART_TRANSMITTER_ENABLE USART_TXEN_bm

/******************************************************************************/
/***   Mode select                                                            */
/******************************************************************************/

/* p.308 */
#define USART_MODE_ASYNC 0x00 /* 00XXXXXX */
#define USART_MODE_SYNC 0x40 /* 01XXXXXX */
#define USART_MODE_IRCOM 0x80 /* 10XXXXXX */
#define USART_MODE_MSPI 0xC0 /* 11XXXXXX */

/******************************************************************************/
/***   Parity                                                                 */
/******************************************************************************/

/* p.308 */
#define USART_PARITY_NONE 0x00 /* XX00XXXX */
#define USART_PARITY_EVEN 0x20 /* XX10XXXX */
#define USART_PARITY_ODD  0x30 /* XX11XXXX */


/******************************************************************************/
/***   Stop bits                                                              */
/******************************************************************************/

/* p.309 */
#define USART_STOP_BITS_1 0x00 /* XXXX0XXX */
#define USART_STOP_BITS_2 0x08 /* XXXX1XXX */

/******************************************************************************/
/***   Character size                                                         */
/******************************************************************************/

/* p.309 */
#define USART_DATA_BITS_5 0x00 /* XXXXX000 */
#define USART_DATA_BITS_6 0x01 /* XXXXX001 */
#define USART_DATA_BITS_7 0x02 /* XXXXX010 */
#define USART_DATA_BITS_8 0x03 /* XXXXX011 */
#define USART_DATA_BITS_9 0x07 /* XXXXX111 */

/******************************************************************************/
/***   Clock polarity                                                         */
/******************************************************************************/

/*
#define USART_RISING_XCKN_EDGE 0x00
#define USART_FALLING_XCKN_EDGE _BV (UCPOL0)
*/

#endif /* #ifndef __RS232_ATXMEGA256A3__ */
