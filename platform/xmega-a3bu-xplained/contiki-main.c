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

#include "radio/rf230bb/rf230bb.h"
#include "net/mac/frame802154.h"
#include "net/mac/framer-802154.h"
#include "net/sicslowpan.h"

#include "contiki-net.h"
#include "contiki-lib.h"

#include "net/rime.h"

/* Begin Code from INGA*/

#ifdef IEEE802154_PANADDR
uint16_t eemem_panaddr EEMEM = IEEE802154_PANADDR;
#else
uint16_t eemem_panaddr EEMEM = 0;
#endif

#ifdef IEEE802154_PANID
uint16_t eemem_panid EEMEM = IEEE802154_PANID;
#else
uint16_t eemem_panid EEMEM = 0xABCD;
#endif


#ifdef NODE_ID
uint16_t eemem_nodeid EEMEM = NODEID;
#else
uint16_t eemem_nodeid EEMEM = 0;
#endif

#ifdef CHANNEL_802_15_4
uint8_t eemem_channel[2] EEMEM = {CHANNEL_802_15_4, ~CHANNEL_802_15_4};
#else
uint8_t eemem_channel[2] EEMEM = {26, ~26};
#endif

#ifdef RF230_MAX_TX_POWER
uint8_t eemem_txpower EEMEM = RF230_MAX_TX_POWER;
#else
uint8_t eemem_txpower EEMEM = 0;
#endif

/* End code from INGA*/

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
	watchdog_start();

	// rtimer	
	rtimer_init();
	
	// Initialize process subsystem
	process_init();

	// etimers must be started before ctimer_init
	process_start(&etimer_process, NULL);

	ctimer_init();
	/* Start radio and radio receive process */

	// @DEBUG
	//PORTR.DIRSET = PIN1_bm;
	//PORTR.OUT |= PIN1_bm;

	printf("before radio init\n");

	NETSTACK_RADIO.init();
	
	rimeaddr_t addr = {0xA, 0x3};
	rimeaddr_set_node_addr(&addr); 
	
	rf230_set_pan_addr(
		eemem_panid, // get_panid_from_eeprom() 
		eemem_panaddr, //get_panaddr_from_eeprom()
		(uint8_t *)&addr.u8
	);
	
	rf230_set_channel(26); //get_channel_from_eeprom()
	rf230_set_txpower(55); // get_txpower_from_eeprom()
	
	
	queuebuf_init();
	NETSTACK_RDC.init();
	NETSTACK_MAC.init();
	NETSTACK_NETWORK.init();

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
