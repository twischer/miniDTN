#include "xmega_clock.h"

void xmega_clock_output(void)
{
	//Enable output of clock on PortC Pin 7 (p.161)
	PORTC.DIR = 0x80; // set PortC.7 as output pin
	PORTCFG.CLKEVOUT = 0x1; // set clock to be on portc.7
}

/**
 * We will use the 2 MHz internal oscillator by default as the source for the PLL
 *
 * The PLL accepts a factor (1 - 31) that can be used to multiply the PLL Source clock.
 * This way we can generate multiplies of a 2 MHz clock
 */

void xmega_clock_init(void)
{
	// 2MHz internal oscillator is selected by default for the PLL (00 in the MSB)	
	// The Constant will be taken from the clock.h
	OSC.PLLCTRL = 0 | XMEGA_CLOCK_CONF_PLL_FACTOR;
	// Enable PLL
	OSC.CTRL |= OSC_PLLEN_bm;
	// Wait for it to stablize
	while ( !(OSC.STATUS & OSC_PLLEN_bm) ) ;
	// Set main system clock to PLL internal clock
	CCP = CCP_IOREG_gc; // Secret handshake so we can change clock
	CLK.CTRL = (CLK.CTRL & ~CLK_SCLKSEL_gm ) | CLK_SCLKSEL_PLL_gc;
}
