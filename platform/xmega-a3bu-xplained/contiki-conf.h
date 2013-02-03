#ifndef __CONTIKI_CONF_H__
#define __CONTIKI_CONF_H__

#include <avr/io.h>
#include <stdint.h>
#include <avrdef.h>

// Only when RTC is defined (--> On Battery Backups we still use the timer)
#ifdef RTC
	// Defining this constant will cause the RTC to be used instead of a HW timer
	// Setting this constant to 0 will use the RTC compare interrupt directly
	// Setting this constant to 1 will use an event channel for timer source
	#define XMEGA_TIMER_RTC 0

	// Enables Clock Output on PortD Pin7
	#define PLATFORM_CLOCKOUT_D7 1
	
	// Force PowerReduction for the RTC to be off
	#undef POWERREDUCTION_CONF_RTC
	#define POWERREDUCTION_CONF_RTC 0

	#include "xmega_rtc.h"
#endif


#ifndef PLATFORM_CONF_RADIO
	#define PLATFORM_RADIO 1
#else
	#define PLATFORM_RADIO PLATFORM_CONF_RADIO
#endif

// include drivers for this platform here because there are #ifdef's in the .h files that we need immedeately
#include "xmega_clock.h"
#include "xmega_interrupt.h"
#include "xmega_powerreduction.h"
#include "xmega_timer.h"
#include "xmega_spi.h"
// #include "xmega_adc.h"

/*************************************************************************/

#define PLATFORM_HAS_LEDS	1

// @TODO this is only for XMega Xplained 
#define LEDS_PxDIR PORTR.DIRSET
#define LEDS_PxOUT PORTR.OUT
#define LEDS_CONF_GREEN PIN0_bm
#define LEDS_CONF_YELLOW PIN1_bm
#define LEDS_CONF_ALL (PIN0_bm | PIN1_bm)

#define WATCHDOG_CONF_TIMEOUT WDT_PER_2KCLK_gc

// RF230 / 231 / 232 / 233

// Radio: RF230/231/232/233
#define SSPORT     C
#define SSPIN      (0x04)
#define SPIPORT    C
#define MOSIPIN    (0x05)
#define MISOPIN    (0x06)
#define SCKPIN     (0x07)
#define RSTPORT    C
#define RSTPIN     (0x00)
#define IRQPORT    C
#define IRQPIN     (0x02)
#define SLPTRPORT  C
#define SLPTRPIN   (0x03)
//#define TXCWPORT   C
//#define TXCWPIN    (0x00)
//#define USART      0
//#define USARTVECT  USART0_RX_vect
// #define TICKTIMER  3

//#define UIP_CONF_IPV6 1

#define ENERGEST_CONF_ON          0

#if RF230BB
	#undef PACKETBUF_CONF_HDR_SIZE 
#endif

#if UIP_CONF_IPV6
	#define RIMEADDR_CONF_SIZE        8
	#define UIP_CONF_ICMP6            1
	#define UIP_CONF_UDP              1
	#define UIP_CONF_TCP              1
	//#define UIP_CONF_IPV6_RPL         0
	#define NETSTACK_CONF_NETWORK       sicslowpan_driver
	#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_HC06
#else
	#define RIMEADDR_CONF_SIZE        2
	#define NETSTACK_CONF_NETWORK     rime_driver
#endif
#define UIP_CONF_DS6_NBR_NBU      20
#define UIP_CONF_DS6_DEFRT_NBU    2
#define UIP_CONF_DS6_PREFIX_NBU   3
#define UIP_CONF_DS6_ROUTE_NBU    20
#define UIP_CONF_DS6_ADDR_NBU     3
#define UIP_CONF_DS6_MADDR_NBU    0
#define UIP_CONF_DS6_AADDR_NBU    0

#define UIP_CONF_LL_802154       1
#define UIP_CONF_LLH_LEN         0

/* 10 bytes per stateful address context - see sicslowpan.c */
/* Default is 1 context with prefix aaaa::/64 */
/* These must agree with all the other nodes or there will be a failure to communicate! */
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS 1
#define SICSLOWPAN_CONF_ADDR_CONTEXT_0 {addr_contexts[0].prefix[0]=0xaa;addr_contexts[0].prefix[1]=0xaa;}
#define SICSLOWPAN_CONF_ADDR_CONTEXT_1 {addr_contexts[1].prefix[0]=0xbb;addr_contexts[1].prefix[1]=0xbb;}
#define SICSLOWPAN_CONF_ADDR_CONTEXT_2 {addr_contexts[2].prefix[0]=0x20;addr_contexts[2].prefix[1]=0x01;addr_contexts[2].prefix[2]=0x49;addr_contexts[2].prefix[3]=0x78,addr_contexts[2].prefix[4]=0x1d;addr_contexts[2].prefix[5]=0xb1;}

/* 211 bytes per queue buffer */
#define QUEUEBUF_CONF_NUM         8

/* 54 bytes per queue ref buffer */
#define QUEUEBUF_CONF_REF_NUM     2

/* Take the default TCP maximum segment size for efficiency and simpler wireshark captures */
/* Use this to prevent 6LowPAN fragmentation (whether or not fragmentation is enabled) */
//#define UIP_CONF_TCP_MSS      48

/* 30 bytes per TCP connection */
/* 6LoWPAN does not do well with concurrent TCP streams, as new browser GETs collide with packets coming */
/* from previous GETs, causing decreased throughput, retransmissions, and timeouts. Increase to study this. */
#define UIP_CONF_MAX_CONNECTIONS 1

/* 2 bytes per TCP listening port */
#define UIP_CONF_MAX_LISTENPORTS 1

/* 25 bytes per UDP connection */
#define UIP_CONF_UDP_CONNS      10

#define UIP_CONF_IP_FORWARD      0
#define UIP_CONF_FWCACHE_SIZE    0

#define UIP_CONF_IPV6_CHECKS     1
#define UIP_CONF_IPV6_QUEUE_PKT  1
#define UIP_CONF_IPV6_REASSEMBLY 0

#define UIP_CONF_UDP_CHECKSUMS   1
#define UIP_CONF_TCP_SPLIT       1
#define UIP_CONF_DHCP_LIGHT      1

#define NETSTACK_CONF_MAC         nullmac_driver
#define NETSTACK_CONF_RDC         nullrdc_driver
#define NETSTACK_CONF_FRAMER      framer_802154
#define NETSTACK_CONF_RADIO       rf230_driver

#define RADIO_CONF_CALIBRATE_INTERVAL 256
/* AUTOACK receive mode gives better rssi measurements, even if ACK is never requested */
#define RF230_CONF_AUTOACK        0
/* Request 802.15.4 ACK on all packets sent (else autoretry). This is primarily for testing. */
#define SICSLOWPAN_CONF_ACK_ALL   0
/* Number of auto retry attempts+1, 1-16. Set zero to disable extended TX_ARET_ON mode with CCA) */
#define RF230_CONF_AUTORETRIES    3
/* Number of CSMA attempts 0-7. 802.15.4 2003 standard max is 5. */
#define RF230_CONF_CSMARETRIES    5
/* CCA theshold energy -91 to -61 dBm (default -77). Set this smaller than the expected minimum rssi to avoid packet collisions */
/* The Jackdaw menu 'm' command is helpful for determining the smallest ever received rssi */
#define RF230_CONF_CCA_THRES    -85
/* Allow 6lowpan fragments (needed for large TCP maximum segment size) */
#define SICSLOWPAN_CONF_FRAG      1
/* Most browsers reissue GETs after 3 seconds which stops fragment reassembly so a longer MAXAGE does no good */
#define SICSLOWPAN_CONF_MAXAGE    3
/* How long to wait before terminating an idle TCP connection. Smaller to allow faster sleep. Default is 120 seconds */
#define UIP_CONF_WAIT_TIMEOUT     5

// RADIO_CHANNEL
#ifndef RADIO_CONF_CHANNEL
	#define RADIO_CHANNEL	26
#else
	#define RADIO_CHANNEL	RADIO_CONF_CHANNEL
#endif

// RADIO_TX_POWER
#ifndef RADIO_CONF_TX_POWER
	#define RADIO_TX_POWER	0
#else
	#define RADIO_TX_POWER	RADIO_CONF_TX_POWER
#endif

// NODE_ID
#ifdef NODE_ID
	#undef NODE_ID
	#warning Use NODE_CONF_ID to define your NodeId
#endif

#ifndef NODE_CONF_ID
	#define NODE_ID	0
#else
	#define NODE_ID	NODE_CONF_ID
#endif

// RADIO_TX_POWER
#ifndef RADIO_CONF_PAN_ID
	#define RADIO_PAN_ID	0xABCD
#else
	#define RADIO_PAN_ID	RADIO_CONF_TX_POWER
#endif

#ifndef RADIO_CONF_PRESCALER
	#define RADIO_PRESCALER SPI_PRESCALER_DIV8_gc
#else
	#define RADIO_PRESCALER RADIO_CONF_PRESCALER
#endif

/*************************************************************************/

// #define RTIMER_ARCH_PRESCALER 1

void platform_radio_init(void);
void platform_enable_clockout(void);

/*************************************************************************/

#define PLATFORM PLATFORM_AVR
#define CCIF
#define CLIF

// This is for serial line IP communication
#define SLIP_PORT RS232_PORT_0

#endif /* __CONTIKI_CONF_H__ */
