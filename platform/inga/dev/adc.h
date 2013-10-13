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
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \mainpage
 *
 * \section about About
 * In addition to the IBRnode sensor node hardware development, a software
 * library for installed sensing and storage stuff was written. A short
 * description and overview is given in chapter 'Software Peripherie Bibliothek'
 * of the project thesis.
 * The software library is divided into several layers, beginning with the low
 * level hardware drivers. Theses drivers set up the communication devices like
 * adc, i2c and spi. The interface layer holds the device specific information about
 * register values and communication protocol. For each sensor and memory device, an
 * interface uses the needed hardware drivers from the lower software layer. Note that
 * this library is just a simple collection of modules to perform working with the
 * the new developed IBRnode.
 * \section usage Usage
 * The software library is full compatible with contiki os and can be directly used
 * in a process by adding the specific header files.
 */

/**
 * \file
 *		ADC driver definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_sensors_driver
 * @{
 */

/**
 * \defgroup adc_driver Analog Digital Converter (internal ADC)
 *
 * <p>The ATmega1284p has 8 ADC-channels. You can run the ADC in
 *    different modes like just "single coversion", "free running" or
 *    "auto triggered" by timer or interrupts. Moreover it is possible
 *    to get differential values of two ADC-channels (with or without
 *    a gain factor)</p>
 * @{
 *
 */

#ifndef ADCCTR_H_
#define ADCCTR_H_

#include <avr/io.h>


/********************************************************************
 * ADC mode:
 ********************************************************************/

/*!
 * mode: ADC Single Conversion
 * \note The ADC will make just one conversion cycle (normal use)
 */
#define ADC_SINGLE_CONVERSION	0xFF
/*!
 * mode: ADC Free Running Mode
 * \note The ADC runs all the time. An interrupt occurs whenever one ADC cycle
 * will finish
 */
#define ADC_FREE_RUNNING		0x00
/*!
 * mode: ADC Analog Comparator
 * \note Sets the ADC to comparator mode.
 */
#define ADC_ANALOG_COMPARATOR	(1 << ADTS0)
/*!
 * mode: External Interrupt Trigger
 * \note Enables the auto trigger mode. The ADC will start a conversion, whenever
 * changes on external interrupt will occur.
 */
#define ADC_EXT_INTERRUPT		(1 << ADTS1)
/*!
 * mode: Timer0 Compare Flag Trigger
 * \note Enables the auto trigger mode. The ADC will start a conversion, whenever
 * Timer0 Compare Flag is set.
 */
#define ADC_TIMER0_COMP_FLAG	((1 << ADTS1) | (1 << ADTS0))
/*!
 * mode: Timer0 Compare Flag Trigger
 * \note Enables the auto trigger mode. The ADC will start a conversion, whenever
 * Timer0 overflow occurs.
 */
#define ADC_TIMER0_OVERFLOW		(1 << ADTS2)
/*!
 * mode: Timer1 Compare Flag Trigger
 * \note Enables the auto trigger mode. The ADC will start a conversion, whenever
 * Timer0 Compare Flag is set.
 */
#define ADC_TIMER1_COMP_FLAG	((1 << ADTS2) | (1 << ADTS0))
/*!
 * mode: Timer1 Compare Flag Trigger
 * \note Enables the auto trigger mode. The ADC will start a conversion, whenever
 * Timer0 overflow occurs.
 */
#define ADC_TIMER1_OVERFLOW		((1 << ADTS2) | (1 << ADTS1))
/*!
 * mode: Timer1 Compare Flag Trigger
 * \note Enables the auto trigger mode. The ADC will start a conversion, whenever
 * Timer0 Capture Flag is set.
 */
#define ADC_TIMER1_CAPTURE		((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0))

/********************************************************************
 * ADC reference voltage source:
 ********************************************************************/

/*!
 * ref: External Reference Voltage (Aref)
 * \note The ADC reference voltage is provided by the external reference pin Aref
 */
#define	ADC_REF_AREF   			0
/*!
 * ref: Supply Voltage Reference (AVcc)
 * \note The ADC reference voltage is provided by the supply voltage.
 */
#define	ADC_REF_AVCC   			(1 << REFS0)
/*!
 * ref: Internal 1.1V Reference
 * \note The ADC reference voltage is provided by internal 1.1V
 */
#define	ADC_REF_1100MV_INT   	(1 << REFS1)
/*!
 * ref: Internal 2.56V Reference
 * \note The ADC reference voltage is provided by internal 2.56V
 */
#define	ADC_REF_2560MV_INT   	((1 << REFS1)| (1 << REFS0))


/**
 * \name ADMUX - ADC Multiplexer Selection Register
 * \{ */
#define ADC_CHANNEL_0			0x00
#define ADC_CHANNEL_1			0x01
#define ADC_CHANNEL_2			0x02
#define ADC_CHANNEL_3			0x03
#define ADC_CHANNEL_4			0x04
#define ADC_CHANNEL_5			0x05
#define ADC_CHANNEL_6			0x06
#define ADC_CHANNEL_7			0x07

#define ADC_DIFF_ADC0_ADC0_10X	0x08
#define ADC_DIFF_ADC1_ADC0_10X	0x09
#define ADC_DIFF_ADC0_ADC0_200X 0x0A
#define ADC_DIFF_ADC1_ADC0_200X 0x0B
#define ADC_DIFF_ADC2_ADC2_10X	0x0C
#define ADC_DIFF_ADC3_ADC2_10X	0x0D
#define ADC_DIFF_ADC2_ADC2_200X	0x0E
#define ADC_DIFF_ADC3_ADC2_200X	0x0F
#define ADC_DIFF_ADC0_ADC1_1X	0x10
#define ADC_DIFF_ADC1_ADC1_1X	0x11
#define ADC_DIFF_ADC2_ADC1_1X	0x12
#define ADC_DIFF_ADC3_ADC1_1X	0x13
#define ADC_DIFF_ADC4_ADC1_1X	0x14
#define ADC_DIFF_ADC5_ADC1_1X	0x15
#define ADC_DIFF_ADC6_ADC1_1X	0x16
#define ADC_DIFF_ADC7_ADC1_1X	0x17
#define ADC_DIFF_ADC0_ADC2_1X	0x18
#define ADC_DIFF_ADC1_ADC2_1X	0x19
#define ADC_DIFF_ADC2_ADC2_1X	0x1A
#define ADC_DIFF_ADC3_ADC2_1X	0x1B
#define ADC_DIFF_ADC4_ADC2_1X	0x1C
/** \} */

/*
 * ADCSRA - ADC control and status register A
 * \cond
 */
#define ADC_ENABLE				(1 << ADEN)

#define ADC_START				(1 << ADSC)
#define ADC_STOP				0x00

#define ADC_LEFT_ADJUSTED		(1 << ADLAR)
#define ADC_RIGHT_ADJUSTED  	(0 << ADLAR)

#define ADC_TRIGGER_ENABLE		(1 << ADATE)
#define ADC_TRIGGER_DISABLE		(0 << ADATE)

#define ADC_INTERRUPT_ENABLE	(1 << ADIE)
#define ADC_INTERRUPT_DISABLE	(0 << ADIE)

#define ADC_PRESCALE_2   		(1 << ADPS0)
#define ADC_PRESCALE_4   		(1 << ADPS1)
#define ADC_PRESCALE_8   		((1 << ADPS0) | (1 << ADPS1))
#define ADC_PRESCALE_16   		(1 << ADPS2)
#define ADC_PRESCALE_32   		((1 << ADPS2) | (1 << ADPS0))
#define ADC_PRESCALE_64   		((1 << ADPS2) | (1 << ADPS1))	/*choose this for F_CPU = 8MHZ*/
#define ADC_PRESCALE_128   		((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))
/*
 * \endcond
 */


/**
 * \brief      Initialize the ADC module
 *
 * \param mode Select an ADC mode like single conversion or
 * 			   free running, etc.
 * \param ref  The ADC reference voltage source.
 *
 */
void adc_init(uint8_t mode, uint8_t ref);

/**
 * \brief      This function returns the ADC data register
 *             value. Use adc_set_mux to preselect the ADC-channel
 *             or differential input.
 *
 * \return     The ADC-value of preselected mux
 *
 */
uint16_t adc_get_value(void);

/**
 * \brief      This function returns the ADC data register
 *             value of the given ADC channel.
 *
 * \param chn  The ADC channel (ADC0 ... ADC7)
 *
 * \return     The ADC-value
 *
 * 			   This function is more efficient for the single
 * 			   conversion mode. Just select the ADC channel (or
 * 			   differential settings) and get the result.
 */
uint16_t adc_get_value_from(uint8_t chn);

/**
 * \brief      With this function you can set the ADX multiplexer.
 * 			   Here you select the ADC channel or choose differential
 *             value between two ADC-channels with or without gain.
 *
 * \param mux  Select single ADC channel (ADC_CHANNEL_0 ... ADC_CHANNEL_7)
 *             or differential input of various ADC-channels with or
 *             without gain
 *
 */
void adc_set_mux(uint8_t mux);

/**
 * \brief      This function stops all running ADC
 *
 */
void adc_deinit(void);

/** @} */
/** @} */

#endif /* ADCCTR_H_ */
