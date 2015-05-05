#include <xmega_powerreduction.h>

/*
	#define __PORT_PULLUP(port, mask) { \
		PORTCFG.MPCMASK = mask ; \
		port.PIN0CTRL = PORT_OPC_PULLUP_gc; \
	}

	__PORT_PULLUP(PORTA, 0xFC); // dont pullup LEDs here (0xFC)
	__PORT_PULLUP(PORTB, 0xFF);
	//__PORT_PULLUP(PORTC, 0xFF); // Dont pullup SPI
	//__PORT_PULLUP(PORTD, 0xFF);
	__PORT_PULLUP(PORTE, 0xFF);
	__PORT_PULLUP(PORTF, 0xDF);
	__PORT_PULLUP(PORTR, 0x03);
	*/

void xmega_pr_set(uint8_t pr)
{
	PR.PRGEN = pr;
}

// Disable JTAG in SW Mode
void xmega_pr_jtag_enable()
{
	MCU_MCUCR = MCU_JTAGD_bm;
}

void xmega_pr_porta_set(uint8_t pr)
{
	PR.PRPA = pr; // Power Reduction Port A
}

void xmega_pr_portb_set(uint8_t pr)
{
	PR.PRPB = pr; // Power Reduction Port B
}

void xmega_pr_dac_enable(void)
{
	xmega_pr_porta_set(PR_DAC_bm);
	xmega_pr_portb_set(PR_DAC_bm);
}

void xmega_pr_adc_enable(void)
{ 
	xmega_pr_porta_set(PR_ADC_bm);
	xmega_pr_portb_set(PR_ADC_bm);
}

void xmega_pr_ac_enable(void)
{
	xmega_pr_porta_set(PR_AC_bm);
	xmega_pr_portb_set(PR_AC_bm);
}

// Disable TWI on Port C,D,E,F
void xmega_pr_twi_enable(void)
{
	PR.PRPC |= PR_TWI_bm;
	PR.PRPD |= PR_TWI_bm;
	PR.PRPE |= PR_TWI_bm;
	PR.PRPF |= PR_TWI_bm;
}

// Disable HiResolution Mode on Port C,D,E,F
void xmega_pr_hires_enable(void)
{
	PR.PRPC |= PR_HIRES_bm;
	PR.PRPD |= PR_HIRES_bm;
	PR.PRPE |= PR_HIRES_bm;
	PR.PRPF |= PR_HIRES_bm;
}

void xmega_pr_usart1_enable(void)
{
	PR.PRPC |= PR_USART1_bm;
	PR.PRPD |= PR_USART1_bm;
	PR.PRPE |= PR_USART1_bm;
	PR.PRPF |= PR_USART1_bm;
}

void xmega_pr_usart0_enable(void)
{
	PR.PRPC |= PR_USART0_bm;
	PR.PRPD |= PR_USART0_bm;
	PR.PRPE |= PR_USART0_bm;
	PR.PRPF |= PR_USART0_bm;
}

void xmega_pr_tc1_enable(void)
{
	PR.PRPC |= PR_TC1_bm;
	PR.PRPD |= PR_TC1_bm;
	PR.PRPE |= PR_TC1_bm;
	PR.PRPF |= PR_TC1_bm;
}

void xmega_pr_tc0_enable(void)
{
	PR.PRPC |= PR_TC0_bm;
	PR.PRPD |= PR_TC0_bm;
	PR.PRPE |= PR_TC0_bm;
	PR.PRPF |= PR_TC0_bm;
}

void xmega_pr_spi_enable(void)
{
	PR.PRPC |= PR_SPI_bm;
	PR.PRPD |= PR_SPI_bm;
	PR.PRPE |= PR_SPI_bm;
	PR.PRPF |= PR_SPI_bm;
}

void xmega_pr_nvm_enable(void)
{
	NVM.CTRLB |= NVM_FPRM_bm | NVM_EPRM_bm; // Enable Flash and EEPROM Power Reduction
}

void xmega_powerreduction_enable(void)
{
	#if POWERREDUCTION_USB
	xmega_pr_set(PR_USB_bm);
	#endif

	#if POWERREDUCTION_AES
	xmega_pr_set(PR_AES_bm);
	#endif

	#if POWERREDUCTION_EBI
	xmega_pr_set(PR_EBI_bm);
	#endif

	#if POWERREDUCTION_DMA
	xmega_pr_set(PR_DMA_bm);
	#endif

	#if POWERREDUCTION_EVSYS
	xmega_pr_set(PR_EVSYS_bm);
	#endif
	
	#if POWERREDUCTION_RTC
	xmega_pr_set(PR_RTC_bm);
	#endif
	
	#ifdef PR_ENABLE
	// power save modes
	// xmega_pr_set(PR_USB_bm | PR_AES_bm | PR_EBI_bm | PR_DMA_bm | PR_EVSYS_bm); // PR_RTC_bm | PR_EVSYS_bm

	#endif
	
	#if POWERREDUCTION_DAC
	xmega_pr_dac_enable();
	#endif

	#if POWERREDUCTION_ADC
	xmega_pr_adc_enable();
	#endif

	#if POWERREDUCTION_AC
	xmega_pr_ac_enable();
	#endif
	
	#if POWERREDUCTION_TWI
	xmega_pr_twi_enable();
	#endif

	#if POWERREDUCTION_HIRES
	xmega_pr_hires_enable();
	#endif

	#if POWERREDUCTION_USART0
	xmega_pr_usart0_enable();
	#endif

	#if POWERREDUCTION_USART1
	xmega_pr_usart1_enable();
	#endif

	#if POWERREDUCTION_TC0
	xmega_pr_tc0_enable();
	#endif

	#if POWERREDUCTION_TC1
	xmega_pr_tc1_enable();
	#endif

	#if POWERREDUCTION_SPI
	xmega_pr_spi_enable();
	#endif
	
	/*
	PR.PRPC |=             PR_USART0_bm; //  - SPI used for Radio Chip - TC0 used for contiki
	PR.PRPD |= PR_TC0_bm | PR_USART0_bm | PR_SPI_bm;
	PR.PRPE |= PR_TC0_bm                | PR_SPI_bm; // PR_USART0 - Usart used as stdout
	PR.PRPF |= PR_TC0_bm                | PR_SPI_bm;
	*/
	
	// Enable JTag PowerReduction in SW Mode
	#if POWERREDUCTION_JTAG
	xmega_pr_jtag_enable();
	#endif
}
