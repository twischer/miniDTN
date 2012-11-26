// #define __PROG_TYPES_COMPAT__

#include "contiki.h"

#include "contiki-conf.h"

#include <avr/pgmspace.h>
#include <avr/fuse.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include <dev/watchdog.h>

#include "interrupt.h"

#include <dev/leds.h>

#include <util/delay.h>

#include <dev/rs232.h>
#include "dev/serial-line.h"
#include "dev/slip.h"

#include <autostart.h>
#include "rtimer-arch.h"

/*
//#include "contiki-net.h"
//#include "contiki-lib.h"

#include "dev/rs232.h"

//#include "clock.h"
//#include <interrupt.h>
//#include <cfs.h>
//#include <sd.h>
//#include <spi-xmega.h>
//

//#include "compiler.h"
//#include "preprocessor.h"
//#include "sysclk.h"



void setup_board(void) {
    //enable relay output
    PORTE.DIR |= (1<<1);

}

*/

#include "drv/uart.h"


#define TIMER_LIMIT 199
volatile int timer = TIMER_LIMIT;

static int i = 0;


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

void setup_clock()
{
    OSC.CTRL |= OSC_RC32MEN_bm;
    while(!(OSC.STATUS & OSC_RC32MRDY_bm));
    CCP = CCP_IOREG_gc;
    CLK.CTRL = 0x01;
}

/*
void timer_init_199Hz()
{
	TCC1.CTRLA = TC_CLKSEL_DIV1024_gc; // Presacler 1024
	TCC1.CTRLB = 0x00; // select Modus: Normal
	TCC1.PER = 157; // Overflow bei 39
	TCC1.CNT = 0x00; // Zurücksetzen auf 0
	TCC1.INTCTRLA = 0b00000011; // Interrupt Highlevel

	PMIC.CTRL |= PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm; 
}
*/


void init(void)
{
	cli();

	PORTR_DIRSET |= PIN0_bm | PIN1_bm; // init LEDs
	PORTR_OUTSET = 0;

	// 2MHz by default, 1024er Divider, Zählen bis 39: 19.97ms
	// 32MHz
	
	// timer_init_199Hz();

	interrupt_init( PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm /*PMIC_CTRL_HML_gm*/ );
	
	rs232_init(RS232_USARTC0, XMEGA_BAUD_ASYNC_9600, USART_PARITY_NONE | USART_STOP_BITS_1 | USART_DATA_BITS_8);
 	rs232_redirect_stdout(RS232_USARTC0);
	
	watchdog_init();

	clock_init();

	
// 
//  	interrupt_start();

	//setup_clock();
	//uart_init();
	
	printf("welcome\n");

	// rtimer_init();
	//printf("rtimer init\n");

	// Initialize process subsystem
	process_init();
	printf("process init\n");

	// etimers must be started before ctimer_init
	process_start(&etimer_process, NULL);
	printf("process_start\n");

	autostart_start(autostart_processes);
	printf("autostart_start\n");

	// system_clock_init();

	sei();
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
