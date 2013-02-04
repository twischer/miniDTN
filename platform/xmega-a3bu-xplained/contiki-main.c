// #define __PROG_TYPES_COMPAT__



#include "contiki.h"

#include "contiki-conf.h"

#include <avr/pgmspace.h>
#include <avr/fuse.h>
#include <stdio.h>
#include <string.h>
#include <dev/watchdog.h>
#include <stdbool.h>

//#include <dev/leds.h>

//#include <util/delay.h>

#include <dev/rs232.h>
#include "dev/serial-line.h"
#include "dev/slip.h"

// settings manager
#include "settings.h"

#include <autostart.h>
#include "rtimer-arch.h"


// Radio
#if RF230BB
	#include "radio/rf230bb/rf230bb.h"
	#include "net/mac/frame802154.h"
	#include "net/mac/framer-802154.h"
	#include "net/sicslowpan.h"
#else
	#error No Xmega radio driver available
#endif

#include "contiki-net.h"
#include "contiki-lib.h"
#include "net/rime.h"

// NodeID
// #include "dev/nodeid.h"

// Apps 
#if defined(APP_SETTINGS_DELETE)
	#include "settings_delete.h"
#elif defined(APP_SETTINGS_SET)
	#include "settings_set.h"
#endif

void platform_radio_init(void)
{
	// Using default or project value as default value
	// NOTE: These variables will always be overwritten when having an eeprom value
	
	uint8_t radio_tx_power = RADIO_TX_POWER;
	uint8_t radio_channel = RADIO_CHANNEL;
	uint16_t pan_id = RADIO_PAN_ID;
	uint16_t pan_addr = NODE_ID;
	uint8_t ieee_addr[8] = {0,0,0,0,0,0,0,0};

	printf("default_channel: %d\n", RADIO_CHANNEL);
	
	// Start radio and radio receive process
	NETSTACK_RADIO.init();
	
	// NODE_ID
	if(settings_check(SETTINGS_KEY_PAN_ADDR, 0) == true)
	{
		pan_addr = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
	}
	else
	{
		printf("NodeID/PanAddr not in EEPROM - using default\n");
	}

	// TX_POWER
	if(settings_check(SETTINGS_KEY_TXPOWER, 0) == true)
	{
		radio_tx_power = settings_get_uint8(SETTINGS_KEY_TXPOWER, 0);
	}
	else
	{
		printf("Radio TXPower not in EEPROM - using default\n");
	}

	// CHANNEL
	if(settings_check(SETTINGS_KEY_CHANNEL, 0) == true)
	{
		radio_channel = settings_get_uint8(SETTINGS_KEY_CHANNEL, 0);
	}
	else
	{
		printf("Radio Channel not in EEPROM - using default\n");
	}
	
	// IEEE ADDR
	// if setting not set or invalid data - generate ieee_addr from node_id 
	if(settings_check(SETTINGS_KEY_EUI64, 0) != true || settings_get(SETTINGS_KEY_EUI64, 0, (void*)&ieee_addr, (size_t*) sizeof(ieee_addr)) != SETTINGS_STATUS_OK)
	{
		ieee_addr[0] = pan_addr & 0xFF;
		ieee_addr[1] = (pan_addr >> 8) & 0xFF;
		ieee_addr[2] = 0;
		ieee_addr[3] = 0;
		ieee_addr[4] = 0;
		ieee_addr[5] = 0;
		ieee_addr[6] = 0;
		ieee_addr[7] = 0;
		
		printf("Radio IEEE Addr not in EEPROM - using default\n");
	}
	
	printf("network_id(pan_id): 0x%X\n", pan_addr);
	printf("node_id(pan_addr): 0x%X\n", pan_addr);
	printf("radio_tx_power: 0x%X\n", radio_tx_power);
	printf("radio_channel: 0x%X\n", radio_channel);
	printf("ieee_addr: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", ieee_addr[0], ieee_addr[1], ieee_addr[2], ieee_addr[3], ieee_addr[4], ieee_addr[5], ieee_addr[6], ieee_addr[7]);
	
	rimeaddr_t addr = {{(pan_addr >> 8) & 0xFF, pan_addr & 0xFF}};
	rimeaddr_set_node_addr(&addr);
	
	rf230_set_pan_addr(
		pan_id, // Network address 2 byte // get_panid_from_eeprom() 
		pan_addr, // PAN ADD 2 Byte // get_panaddr_from_eeprom()
		(uint8_t *) &ieee_addr // MAC ADDRESS 8 byte
	);
	
	rf230_set_channel(radio_channel);
	rf230_set_txpower(radio_tx_power);

	// Copy NodeID to the link local address
	#if UIP_CONF_IPV6
	memcpy(&uip_lladdr.addr, &addr.u8, sizeof(rimeaddr_t));
	#endif
	
	queuebuf_init();
	NETSTACK_RDC.init();
	NETSTACK_MAC.init();
	NETSTACK_NETWORK.init();
}

/* End code from INGA*/

void init(void)
{
	cli();

	// Init here because we might disable some PR afterwards
	xmega_powerreduction_enable();
	
	// enable High, Med and Low interrupts
	xmega_interrupt_enable(PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm);
	
	// init clock
	xmega_clock_init();
	
	// init RS232
	// Disable PR for Port first
	// PORT0 = E, PORT1 = F
	PR.PRPF &= (~PR_USART0_bm);
	// actual init
	rs232_init(RS232_PORT_1, USART_BAUD_115200, USART_PARITY_NONE | USART_STOP_BITS_1 | USART_DATA_BITS_8);
	rs232_redirect_stdout(RS232_PORT_1);
	
	// init the timer only when we have a batterybackup system, else we use 1kHz from RTC
	#ifdef RTC32
	PR.PRPC &= (~PR_TC0_bm);
	clock_init();
	#endif
	
	// watchdog
	watchdog_init();
	watchdog_start();

	// rtimer (init only when RTC is available) if so we use it as etimer source
	#ifdef RTC
	rtimer_init();
	#endif

	// Event System used for Timer
	#if defined(XMEGA_TIMER_RTC) && XMEGA_TIMER_RTC == 1
	PR.PRGEN &= (~PR_EVSYS_bm);
	EVSYS.CH0MUX = (1 << 3); // Use RTC overflow interrupt as Channel 0 Source
	#endif
	
	// Initialize process subsystem
	process_init();

	// etimers must be started before ctimer_init
	process_start(&etimer_process, NULL);

	ctimer_init();

	//printf("RST.STATUS: %X\n", RST.STATUS);

	#if PLATFORM_RADIO
	// Disable Power Reduction for the SPI
	PR.PRPC &= (~PR_SPI_bm);
	// Init radio
	platform_radio_init();
	#endif
	
	// Enable Non Volatile Memory Power Reduction after we read from EEPROM
	#if POWERREDUCTION_NVM
	xmega_pr_nvm_enable();
	#endif

	// printf("Welcome to Contiki.XMega | F_CPU = %d\n", (unsigned long) F_CPU);

	// autostart processes
	autostart_start(autostart_processes);
	
	#if defined(APP_SETTINGS_SET)
		process_start(&settings_set_process, NULL);
	#endif
	
	#if defined(APP_SETTINGS_DELETE)
		process_start(&settings_delete_process, NULL);
	#endif

	#if UIP_CONF_IPV6
	process_start(&tcpip_process, NULL);
	#endif

	
	
	


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
