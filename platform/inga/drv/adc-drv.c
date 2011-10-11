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
 * \addtogroup adc_driver
 * @{
 */

/**
 * \file
 *      ADC driver implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

#include "adc-drv.h"

void adc_init(uint8_t mode, uint8_t ref){
	ADCSRA = ((ADC_ENABLE) | (ADC_PRESCALE_64));
	ADCSRB = 0x00;
	ADMUX = ref;

	if(mode != ADC_SINGLE_CONVERSION){
		ADCSRB |= (0x07 & mode);
		ADCSRA |= ((ADC_TRIGGER_ENABLE) | (ADC_INTERRUPT_ENABLE));
	}
}

void adc_set_mux(uint8_t mux){
	static uint8_t used_adcs = 0;
	/*save energy by disabling the i/o input buffer*/
	if(mux < 8){
		used_adcs |= (1 << mux);
		DIDR0 |= used_adcs;
	}
	ADMUX &= (0xE0);
	ADMUX |= mux;
	ADCSRA |= ADC_START;
}

uint16_t adc_get_value(void) {
	if(ADCSRA & ADC_TRIGGER_ENABLE){
		/*just read the ADC data register*/
		return ADCW;
	}else{
		/*start single conversion*/
		while (ADCSRA & (1 << ADSC)) {
		 ;
		}
		return ADCW;
	}
}

uint16_t adc_get_value_from(uint8_t chn){
 adc_set_mux(chn);
 return adc_get_value();
}

void adc_deinit(void){
	ADCSRA = ADC_STOP;
	ADCSRB = ADC_STOP;
	ADMUX = ADC_STOP;
}
