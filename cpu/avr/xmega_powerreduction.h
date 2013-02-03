#ifndef __XMEGA_POWERREDUCTION_H__
#define __XMEGA_POWERREDUCTION_H__

#include <avr/io.h>
#include <stdint.h>
#include <avrdef.h>

// !! For some reason this is not defined in iox?????.h
#ifndef PR_USB_bm
#define PR_USB_bm (1 << 6)
#endif

#ifndef POWERREDUCTION_CONF_DAC
	#define POWERREDUCTION_DAC 1
#else
	#define POWERREDUCTION_DAC POWERREDUCTION_DAC
#endif

#ifndef POWERREDUCTION_CONF_ADC
	#define POWERREDUCTION_ADC 1
#else
	#define POWERREDUCTION_ADC POWERREDUCTION_ADC
#endif

#ifndef POWERREDUCTION_CONF_AC
	#define POWERREDUCTION_AC 1
#else
	#define POWERREDUCTION_AC POWERREDUCTION_AC
#endif

#ifndef POWERREDUCTION_CONF_TWI
	#define POWERREDUCTION_TWI 1
#else
	#define POWERREDUCTION_TWI POWERREDUCTION_TWI
#endif

#ifndef POWERREDUCTION_CONF_HIRES
	#define POWERREDUCTION_HIRES 1
#else
	#define POWERREDUCTION_HIRES POWERREDUCTION_HIRES
#endif

#ifndef POWERREDUCTION_CONF_SPI
	#define POWERREDUCTION_SPI 1
#else
	#define POWERREDUCTION_SPI POWERREDUCTION_SPI
#endif

#ifndef POWERREDUCTION_CONF_USB
	#define POWERREDUCTION_USB 1
#else
	#define POWERREDUCTION_USB POWERREDUCTION_USB
#endif

#ifndef POWERREDUCTION_CONF_AES
	#define POWERREDUCTION_AES 1
#else
	#define POWERREDUCTION_AES POWERREDUCTION_AES
#endif

#ifndef POWERREDUCTION_CONF_EBI
	#define POWERREDUCTION_EBI 1
#else
	#define POWERREDUCTION_EBI POWERREDUCTION_EBI
#endif

#ifndef POWERREDUCTION_CONF_DMA
	#define POWERREDUCTION_DMA 1
#else
	#define POWERREDUCTION_DMA POWERREDUCTION_DMA
#endif

#ifndef POWERREDUCTION_CONF_EVSYS
	#define POWERREDUCTION_EVSYS 1
#else
	#define POWERREDUCTION_EVSYS POWERREDUCTION_EVSYS
#endif

#ifndef POWERREDUCTION_CONF_RTC
	#define POWERREDUCTION_RTC 1
#else
	#define POWERREDUCTION_RTC POWERREDUCTION_RTC
#endif

void xmega_pr_set(uint8_t pr);

void xmega_pr_porta_set(uint8_t pr);
void xmega_pr_portb_set(uint8_t pr);

void xmega_pr_dac_enable(void);
void xmega_pr_adc_enable(void);
void xmega_pr_ac_enable(void);
void xmega_pr_twi_enable(void);
void xmega_pr_hires_enable(void);
void xmega_pr_usart1_enable(void);
void xmega_pr_tc1_enable(void);

void xmega_pr_nvm_enable(void);

void xmega_pr_jtag_enable();

#endif // #ifndef __XMEGA_POWERREDUCTION_H__
