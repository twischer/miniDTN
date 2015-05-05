/*
 * Copyright (c) 2006, Technical University of Munich
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
 *
 * @(#)$$
 */

/**
 * \file
 *         Configuration for INGA platform
 *
 * \author
 *         Simon Barner <barner@in.tum.de>
 *         David Kopf <dak664@embarqmail.com>
 */

#ifndef __CONTIKI_CONF_H__
#define __CONTIKI_CONF_H__

/* MCU and clock rate */
#include <stdint.h>
#include "platform-conf.h"

#include <avr/eeprom.h>

/* Skip the last four bytes of the EEPROM, to leave room for things
 * like the avrdude erase count and bootloader signaling. */
#define EEPROM_CONF_SIZE   ((E2END + 1) - 4)

/* @todo: Just a temporary solution... */
#define CFS_CONF_OFFSET_SIZE  uint32_t

/* Maximum timer interval for 16 bit clock_time_t */
#define INFINITE_TIME 0xffff

#ifndef PLATFORM_CONF_RADIO
	#define PLATFORM_RADIO 1
#else
	#define PLATFORM_RADIO PLATFORM_CONF_RADIO
#endif

/* Maximum tick interval is 0xffff/128 = 511 seconds */
#define RIME_CONF_BROADCAST_ANNOUNCEMENT_MAX_TIME INFINITE_TIME/CLOCK_CONF_SECOND /* Default uses 600 */
#define COLLECT_CONF_BROADCAST_ANNOUNCEMENT_MAX_TIME INFINITE_TIME/CLOCK_CONF_SECOND /* Default uses 600 */

/* The 1284p can use TIMER2 with the external 32768Hz crystal to keep time. Else TIMER0 is used. */
/* The sleep timer in raven-lcd.c also uses the crystal and adds a TIMER2 interrupt routine if not already define by clock.c */
#define AVR_CONF_USE32KCRYSTAL 1

/* Rtimer is implemented through the 16 bit Timer1, clocked at F_CPU through a 1024 prescaler. */
/* This gives 7812 counts per second, 128 microsecond precision and maximum interval 8.388 seconds. */
/* Change clock source and prescaler for greater precision and shorter maximum interval. */
/* 0 will disable the Rtimer code */
//#define RTIMER_ARCH_PRESCALER 256UL /*0, 1, 8, 64, 256, 1024 */

/* COM port to be used for SLIP connection. */
#define SLIP_PORT RS232_PORT_0

/* Pre-allocated memory for loadable modules heap space (in bytes)*/
/* Default is 4096. Currently used only when elfloader is present. Not tested on Inga */
//#define MMEM_CONF_SIZE 256

/* Starting address for code received via the codeprop facility. Not tested on Inga */
//#define EEPROMFS_ADDR_CODEPROP 0x8000

/* RADIO_CONF_CALIBRATE_INTERVAL is used in rf230bb and clock.c. If nonzero a 256 second interval is used */
/* Calibration is automatic when the radio wakes so is not necessary when the radio periodically sleeps */
//#define RADIO_CONF_CALIBRATE_INTERVAL 256

/* RADIOSTATS is used in rf230bb, clock.c and the webserver cgi to report radio usage */
#define RADIOSTATS                1

/* More extensive stats */
#define ENERGEST_CONF_ON          0

/* Possible watchdog timeouts depend on mcu. Default is WDTO_2S. -1 Disables the watchdog. */
/* AVR Studio simulator tends to reboot due to clocking the WD 8 times too fast */
//#define WATCHDOG_CONF_TIMEOUT -1

/* Debugflow macro, useful for tracing path through mac and radio interrupts */
//#define DEBUGFLOWSIZE 128


/* ************************************************************************** */
/* Network setup                                                              */
/* ************************************************************************** */

/* Network setup. The new NETSTACK interface requires RF230BB (as does ip4) */
#if RF230BB
#undef PACKETBUF_CONF_HDR_SIZE                  //Use the packetbuf default for header size
#else /* RF230BB */
#define PACKETBUF_CONF_HDR_SIZE   0            //RF230 combined driver/mac handles headers internally
#endif /* RF230BB */


/* 211 bytes per queue buffer */
#define QUEUEBUF_CONF_NUM         8
/* 54 bytes per queue ref buffer */
#define QUEUEBUF_CONF_REF_NUM     2

/* -- Default network stack */

#ifndef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     nullmac_driver
#endif /* NETSTACK_CONF_MAC */

#ifndef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     nullrdc_driver
#endif /* NETSTACK_CONF_RDC */

#ifndef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO   rf230_driver
#endif /* NETSTACK_CONF_RADIO */

#ifndef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER  framer_802154
#endif /* NETSTACK_CONF_FRAMER */

/*
 * Network stack setup.
 */
#if NETSTACK_CONF_WITH_IPV6
#define NETSTACK_CONF_NETWORK     sicslowpan_driver

#define LINKADDR_CONF_SIZE        8

/* -- UIP IPv6 settings */
#define UIP_CONF_ICMP6            1
#define UIP_CONF_IPV6_CHECKS      1
#define UIP_CONF_IPV6_QUEUE_PKT   1
#define UIP_CONF_IPV6_REASSEMBLY  0
/* Most browsers reissue GETs after 3 seconds which stops fragment reassembly
 * so a longer MAXAGE does no good */
#define SICSLOWPAN_CONF_MAXAGE    3
/* Request 802.15.4 ACK on all packets sent (else autoretry).
 * This is primarily for testing. */
#define SICSLOWPAN_CONF_ACK_ALL   0
/* 10 bytes per stateful address context - see sicslowpan.c */
/* Default is 1 context with prefix aaaa::/64 */
/* These must agree with all the other nodes or there will be a failure to communicate! */
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS 1
#define SICSLOWPAN_CONF_ADDR_CONTEXT_0 {addr_contexts[0].prefix[0]=0xaa;addr_contexts[0].prefix[1]=0xaa;}
#define SICSLOWPAN_CONF_ADDR_CONTEXT_1 {addr_contexts[1].prefix[0]=0xbb;addr_contexts[1].prefix[1]=0xbb;}
#define SICSLOWPAN_CONF_ADDR_CONTEXT_2 {addr_contexts[2].prefix[0]=0x20;addr_contexts[2].prefix[1]=0x01;addr_contexts[2].prefix[2]=0x49;addr_contexts[2].prefix[3]=0x78,addr_contexts[2].prefix[4]=0x1d;addr_contexts[2].prefix[5]=0xb1;}

#else /* NETSTACK_CONF_WITH_IPV6 */
/* ip4 should build but is largely untested */
#if NETSTACK_CONF_WITH_DTN
#define NETSTACK_CONF_NETWORK     dtn_network_driver 
#else
#define NETSTACK_CONF_NETWORK     rime_driver
#endif

#define LINKADDR_CONF_SIZE        2

#endif /* NETSTACK_CONF_WITH_IPV6 */

/* -- Radio driver settings */
#define CHANNEL_802_15_4          26
#define RADIO_CONF_CALIBRATE_INTERVAL 256
/* AUTOACK receive mode gives better rssi measurements,
 * even if ACK is never requested */
#define RF230_CONF_AUTOACK        1
/* Make nullrdc wait for the proper ACK before proceeding */
#define NULLRDC_CONF_802154_AUTOACK 1
/* Let the RF230 radio driver generate fake acknowledgements to make nullrdc happy */
#define RF320_CONF_INSERTACK      1
/* Number of CSMA attempts 0-7. 802.15.4 2003 standard max is 5. */
#define RF230_CONF_FRAME_RETRIES  5
/* CCA theshold energy -91 to -61 dBm (default -77).
 * Set this smaller than the expected minimum rssi to avoid packet collisions */
/* The Jackdaw menu 'm' command is helpful for determining the smallest ever received rssi */
#define RF230_CONF_CCA_THRES      -85
/* Default is one RAM buffer for received packets. 
 * More than one may benefit multiple TCP connections or ports */
#define RF230_CONF_RX_BUFFERS     3

/* -- UIP settings */
#define UIP_CONF_UDP_CHECKSUMS    1
/* How long to wait before terminating an idle TCP connection.
 * Smaller to allow faster sleep. Default is 120 seconds */
#define UIP_CONF_WAIT_TIMEOUT     5
/* Take the default TCP maximum segment size for efficiency and simpler wireshark captures */
/* Use this to prevent 6LowPAN fragmentation (whether or not fragmentation is enabled) */
//#define UIP_CONF_TCP_MSS      48

/* 6LoWPAN does not do well with concurrent TCP streams, 
 * as new browser GETs collide with packets coming
 * from previous GETs, causing decreased throughput, retransmissions, and timeouts. 
 * Increase to study this. */
#define UIP_CONF_MAX_CONNECTIONS  1

/* 2 bytes per TCP listening port */
#define UIP_CONF_MAX_LISTENPORTS  1

/* 25 bytes per UDP connection */
#define UIP_CONF_UDP_CONNS        10

#define UIP_CONF_IP_FORWARD       0
#define UIP_CONF_FWCACHE_SIZE     0

#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE      240
#endif

#define UIP_CONF_DHCP_LIGHT       1
/* uip uses 802.15.4 adresses */
#define UIP_CONF_LL_802154        1
/* no link level header */
#define UIP_CONF_LLH_LEN          0

#define UIP_CONF_TCP_SPLIT        1

#include "inga-conf.h"

/* ************************************************************************** */
/* RPL Settings                                                               */
/* ************************************************************************** */
#if UIP_CONF_IPV6_RPL

/* Define MAX_*X_POWER to reduce tx power and ignore weak rx packets 
 * for testing a miniature multihop network.
 * Leave undefined for full power and sensitivity.
 * tx=0 (3dbm, default) to 15 (-17.2dbm)
 * RF230_CONF_AUTOACK sets the extended mode using the energy-detect register 
 * with rx=0 (-91dBm) to 84 (-7dBm)
 * else the rssi register is used having range 0 (91dBm) to 28 (-10dBm)
 * For simplicity RF230_MIN_RX_POWER is based on the energy-detect value 
 * and divided by 3 when autoack is not set.
 * On the RF230 a reduced rx power threshold will not prevent autoack 
 * if enabled and requested.
 * These numbers applied to both Raven and Jackdaw give a maximum 
 * communication distance of about 15 cm
 * and a 10 meter range to a full-sensitivity RF230 sniffer.
#define RF230_MAX_TX_POWER 15
#define RF230_MIN_RX_POWER 30
 */

#define UIP_CONF_ROUTER                 1
#define UIP_CONF_ND6_SEND_RA            0
#define UIP_CONF_ND6_REACHABLE_TIME     600000 /// TODO: fix in default config?
#define UIP_CONF_ND6_RETRANS_TIMER      10000  /// TODO: fix in default config?

#undef UIP_CONF_UDP_CONNS
#define UIP_CONF_UDP_CONNS       12
#undef UIP_CONF_FWCACHE_SIZE
#define UIP_CONF_FWCACHE_SIZE    30
#define UIP_CONF_BROADCAST       1
#define UIP_ARCH_IPCHKSUM        1
#define UIP_CONF_PINGADDRCONF    0
#define UIP_CONF_LOGGING         0

#endif /* UIP_CONF_IPV6_RPL */

/* Logging adds 200 bytes to program size */
#define LOG_CONF_ENABLED         1

/* Contiki Core Interface (has no function here) */
#define CCIF
/* Contiki Loadable Interface (has no function here) */
#define CLIF

#ifndef CC_CONF_INLINE
#define CC_CONF_INLINE inline
#endif 

/* Include the project config.
 * PROJECT_CONF_H might be defined in the project Makefile */
#ifdef PROJECT_CONF_H
#include PROJECT_CONF_H
#endif /* PROJECT_CONF_H */

#endif /* __CONTIKI_CONF_H__ */
