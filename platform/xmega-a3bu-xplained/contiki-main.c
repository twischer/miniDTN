// #define __PROG_TYPES_COMPAT__



#include "contiki.h"

#include "contiki-conf.h"

#include <avr/pgmspace.h>
#include <avr/fuse.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include <dev/watchdog.h>

//#include <dev/leds.h>

//#include <util/delay.h>

#include <dev/rs232.h>
#include "dev/serial-line.h"
#include "dev/slip.h"

#include <autostart.h>
#include "rtimer-arch.h"

// TESTING

/*
#include "radio/rf230bb/rf230bb.h"
#include "net/mac/frame802154.h"
#include "net/mac/framer-802154.h"
#include "net/sicslowpan.h"
*/

/*
ISR(TCC1_OVF_vect) 
{	
	//timer--;
	//if(timer <= 0)
	//{
	//	timer = TIMER_LIMIT;
	//	PORTR_OUTTGL = PIN1_bm;
	//	printf("toggle %10d\n", i++);
	//}
}
*/

/*
void timer_init_199Hz()
{
	TCC1.CTRLA = TC_CLKSEL_DIV1024_gc; // Presacler 1024
	TCC1.CTRLB = 0x00; // select Modus: Normal
	TCC1.PER = 157; // Overflow bei 39
	TCC1.CNT = 0x00; // Zurücksetzen auf 0
	TCC1.INTCTRLA = 0b00000011; // Interrupt Highlevel

	
}
*/

/*
void setup_clock()
{
    OSC.CTRL |= XMEGA_OSC_SOURCE; // enable Oscillator
    while(!(OSC.STATUS & XMEGA_OSC_READY)); // Wait for the oscillator to stablize

//    DFLLRC32M.COMP1 = 0x12;
//    DFLLRC32M.COMP2 = 0x7A;
  

//    OSC.DFLLCTRL = 0x02;
//    DFLLRC32M.CTRL = 1;

    //OSC.DFLLCTRL = (0 << OSC_RC32MCREF_bp);
    //DFLLRC32M.CTRL = DFLL_ENABLE_bm;
    
    CCP = CCP_IOREG_gc;
    CLK.CTRL = XMEGA_CLK_SOURCE;
}
*/

void init(void)
{
	cli();
	
	// enable High, Med and Low interrupts
	xmega_interrupt_enable(PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm);
	
	// init clock
	xmega_clock_init();
	// init timer
	clock_init();

	// init RS232
	rs232_init(RS232_PORT_0, USART_BAUD_9600, USART_PARITY_NONE | USART_STOP_BITS_1 | USART_DATA_BITS_8);
 	rs232_redirect_stdout(RS232_PORT_0);

	// watchdog
	watchdog_init();
	//watchdog_start();

	// rtimer	
	rtimer_init();
	
	// Initialize process subsystem
	process_init();

	// etimers must be started before ctimer_init
	process_start(&etimer_process, NULL);

	// autostart processes
	autostart_start(autostart_processes);

	sei();

	printf("Welcome to Contiki.XMega\n");
}

int main(void)
{
	init();

	while(1)
	{
		watchdog_periodic();
		process_run();
	}
	
	return 0;
}
