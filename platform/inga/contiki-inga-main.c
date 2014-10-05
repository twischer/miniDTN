/*
 * Copyright (c) 2014, TU Braunschweig
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
 */

/**
 * \file Contiki system setup for INGA
 * \author Robert Hartung
 * \author Enrico Joerns <joerns@ibr.cs.tu-bs.de>
 */

#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)



/* If defined 1, prints debug infos. */
#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define PRINTD(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTD(...)
#endif

/* Track interrupt flow through mac, rdc and radio driver */
#if DEBUGFLOWSIZE
uint8_t debugflowsize, debugflow[DEBUGFLOWSIZE];
#define DEBUGFLOW(c) if (debugflowsize<(DEBUGFLOWSIZE-1)) debugflow[debugflowsize++]=c
#else
#define DEBUGFLOW(c)
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "dev/watchdog.h"

// settings manager
#include "lib/settings.h"

// sensors
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include "dev/acc-sensor.h"
#include "dev/gyro-sensor.h"
#include "dev/pressure-sensor.h"
#include "dev/battery-sensor.h"
#include "dev/mag-sensor.h"
#include "dev/at45db.h"

#include "ip/uip.h"

#include "radio/rf230bb/rf230bb.h"
#include "net/mac/frame802154.h"
#include "net/mac/framer-802154.h"
#include "net/ipv6/sicslowpan.h"

#include "net/rime/rime.h"

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"
#include "sys/node-id.h"

#include "dev/rs232.h"
#include "dev/serial-line.h"
#include "dev/slip.h"

#ifdef COFFEE_FILES
#include "cfs/coffee/cfs-coffee.h"
#endif

#if UIP_CONF_IPV6
#include "net/ipv6/uip-ds6.h"
// function declaration for net/uip-debug.c
void uip_debug_ipaddr_print(const uip_ipaddr_t *addr);
void uip_debug_lladdr_print(const uip_lladdr_t *addr);
#endif /* UIP_CONF_IPV6 */


// Apps 
#if (APP_SETTINGS_DELETE == 1)
#include "settings_delete.h"
#endif
#if (APP_SETTINGS_SET == 1)
#include "settings_set.h"
#endif


#if INGA_BOOTSCREEN

#define PRINTA(FORMAT,args...) printf_P(PSTR(FORMAT),##args)

#if INGA_TERMINAL_ESCAPES
#define PRINTA_SEC(a) PRINTA("\e[1m" a "\e[0m")
#else /* INGA_TERMINAL_ESCAPES */
#define PRINTA_SEC(a) PRINTA(a)
#endif /* INGA_TERMINAL_ESCAPES */

#endif /* INGA_BOOTSCREEN */

/*-------------------------------------------------------------------------*/
/*----------------------Configuration of the .elf file---------------------*/
#if (__AVR_LIBC_VERSION__ >= 10700UL)
/* The proper way to set the signature is */
#include <avr/signature.h>
#else

/* signature API not available before avr-lib-1.7.0. Do it manually.*/
typedef struct {
  const unsigned char B2;
  const unsigned char B1;
  const unsigned char B0;
} __signature_t;
#define SIGNATURE __signature_t __signature __attribute__((section (".signature")))
SIGNATURE = {
  .B2 = 0x05, //SIGNATURE_2, //ATMEGA1284p
  .B1 = 0x97, //SIGNATURE_1, //128KB flash
  .B0 = 0x1E, //SIGNATURE_0, //Atmel
};
#endif /* (__AVR_LIBC_VERSION__ >= 10700UL) */

#if INGA_ADC_RANDOM
#include "dev/adc.h"
/** Get a pseudo random number using the ADC. */
static uint8_t
rng_get_uint8(void)
{
  uint8_t i, j = 0;
  for( i = 0; i < 4; i++) {
    adc_init(ADC_SINGLE_CONVERSION, ADC_REF_2560MV_INT);
    adc_set_mux(0x1E);
    int val = adc_get_value();
    j = (j << 2) + val;
    adc_deinit();
    _delay_ms(1); // seems to improve noise
  }
  return j;
}
/*----------------------------------------------------------------------------*/
// NOTE: fixed parts are for 802.15.4 -> Ethernet MAC address matching
// according to [RFC 2373, Appendix A].
static void
generate_new_eui64(uint8_t eui64[8])
{
  eui64[0] = rng_get_uint8() | 0x02; // set U/L bit to 1 (local!)
  eui64[1] = rng_get_uint8();
  eui64[2] = rng_get_uint8();
  eui64[3] = 0xFF;
  eui64[4] = 0xFE;
  eui64[5] = rng_get_uint8();
  eui64[6] = rng_get_uint8();
  eui64[7] = rng_get_uint8();
}
#endif
/*----------------------------------------------------------------------------*/
// implements log function from uipopt.h
void
uip_log(char *msg)
{
  printf("%s\n", msg);
}
/*----------------------------------------------------------------------------*/
struct inga_config_s {
  uint8_t eui64_addr[8];
  uint8_t radio_channel;
  uint8_t radio_tx_power;
  uint16_t pan_id;
  uint16_t pan_addr; // short address
};
struct inga_config_s inga_cfg; // Keep global!
/*----------------------------------------------------------------------------*/
// implement sys/node-id.h interface
unsigned short node_id = NODE_ID;
/*----------------------------------------------------------------------------*/
void
node_id_restore(void)
{
  node_id = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
}
/*----------------------------------------------------------------------------*/
void
node_id_burn(unsigned short node_id)
{
  if (settings_set_uint16(SETTINGS_KEY_PAN_ADDR, (uint16_t) node_id) == SETTINGS_STATUS_OK) {
    uint16_t settings_nodeid = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
    PRINTF("New Node ID: %04X\n", settings_nodeid);
  } else {
    PRINTF("Error: Error while writing NodeID to EEPROM\n");
  }
}
/*----------------------------------------------------------------------------*/
#if INGA_CONVERTTXPOWER  //adds ~120 bytes to program flash size
// NOTE: values for AT86RF231
const char txonesdigit[16]   PROGMEM = {'3','2','2','1','1','0','0','1','2','3','4','5','7','9','2','7'};
const char txtenthsdigit[16] PROGMEM = {'0','8','3','8','3','7','0','0','0','0','0','0','0','0','0','0'};
static void
printtxpower(void)
{
  uint8_t power = rf230_get_txpower() & 0xf;
  char sign = (power < 0x7 ? '+' : '-');
  char tens = (power > 0xD ? '1' : '0');
  char ones = pgm_read_byte(&txonesdigit[power]);
  char tenths = pgm_read_byte(&txtenthsdigit[power]);
  if (tens == '0') {
    printf_P(PSTR("%c%c.%cdBm"), sign, ones, tenths);
  } else {
    printf_P(PSTR("%c%c%c.%cdBm"), sign, tens, ones, tenths);
  }
}
#endif /* INGA_CONVERTTXPOWER */
/*----------------------------------------------------------------------------*/
#if INGA_TERMINAL_ESCAPES
const char msg_ok[]   PROGMEM = "\e[32m[OK]\e[0m\n"; // green
const char msg_err[]   PROGMEM = "\e[31m[N/A]\e[0m\n"; // red
#else /* INGA_TERMINAL_ESCAPES */
const char msg_ok[]   PROGMEM = "[OK]\n";
const char msg_err[]  PROGMEM = "[N/A]\n";
#endif /* INGA_TERMINAL_ESCAPES */

#if INGA_BOOTSCREEN_SENSORS
#define CHECK_SENSOR(name, sensor) \
  printf_P(PSTR("  " name ": ")); \
  printf_P(sensor.status(SENSORS_READY) ? msg_ok : msg_err);
#endif
/*----------------------------------------------------------------------------*/
#define eui64_is_null(eui64) \
    ((eui64[0] == 0x00) \
    && (eui64[1] == 0x00) \
    && (eui64[2] == 0x00) \
    && (eui64[3] == 0x00) \
    && (eui64[4] == 0x00) \
    && (eui64[5] == 0x00) \
    && (eui64[6] == 0x00) \
    && (eui64[7] == 0x00))
/*----------------------------------------------------------------------------*/
#define pan_addr_from_eui64(eui64) \
  (uint16_t) (eui64[0] + eui64[1] + eui64[2] + eui64[3] + eui64[4] + eui64[5] + eui64[6] + eui64[7]);
/*----------------------------------------------------------------------------*/
static void
load_config(void)
{

  // PAN_ID
#ifndef INGA_CONF_PAN_ID
  if (settings_check(SETTINGS_KEY_PAN_ID, 0) == true) {
    inga_cfg.pan_id = settings_get_uint16(SETTINGS_KEY_PAN_ID, 0);
  } else {
    inga_cfg.pan_id = INGA_PAN_ID;
    PRINTD("PanID not in EEPROM - using default (0x%04x)\n", inga_cfg.pan_id);
  }
#else /* INGA_CONF_PAN_ID */
  inga_cfg.pan_id = INGA_PAN_ID;
#endif /* INGA_CONF_PAN_ID */

  /* Radio TX Power */
#ifndef INGA_CONF_RADIO_TX_POWER
  if (settings_check(SETTINGS_KEY_TXPOWER, 0) == true) {
    inga_cfg.radio_tx_power = settings_get_uint8(SETTINGS_KEY_TXPOWER, 0);
  } else {
    inga_cfg.radio_tx_power = INGA_RADIO_TX_POWER;
    PRINTD("Radio TXPower not in EEPROM - using default (%d)\n", inga_cfg.radio_tx_power);
  }
#else /* INGA_CONF_RADIO_TX_POWER */
  inga_cfg.radio_tx_power = INGA_RADIO_TX_POWER;
#endif /* INGA_CONF_RADIO_TX_POWER */

  /* Channel */
#ifndef INGA_CONF_RADIO_CHANNEL
  if (settings_check(SETTINGS_KEY_CHANNEL, 0) == true) {
    inga_cfg.radio_channel = settings_get_uint8(SETTINGS_KEY_CHANNEL, 0);
  } else {
    inga_cfg.radio_channel = INGA_RADIO_CHANNEL;
    PRINTD("Radio Channel not in EEPROM - using default (%d)\n", inga_cfg.radio_channel);
  }
#else /* INGA_CONF_RADIO_CHANNEL */
  inga_cfg.radio_channel = INGA_RADIO_CHANNEL;
#endif /* INGA_CONF_RADIO_CHANNEL */

  /* Overwrite with node id if set */
  if (node_id > 0) {
#if UIP_CONF_IPV6
    memset(inga_cfg.eui64_addr, 0, sizeof(inga_cfg.eui64_addr));
    inga_cfg.eui64_addr[0] |= 0x02; // set U/L bit to 1 (local!)
    inga_cfg.eui64_addr[6] = (node_id >> 8);
    inga_cfg.eui64_addr[7] = node_id & 0xFF;
#else /* UIP_CONF_IPV6 */
    inga_cfg.eui64_addr[0] = node_id & 0xFF;
    inga_cfg.eui64_addr[1] = (node_id >> 8);
#endif /* UIP_CONF_IPV6 */
    inga_cfg.pan_addr = node_id;

    /* Configuration done */
    return;
  }

  /* EUI64 ADDR */
#ifndef INGA_CONF_EUI64
  if (settings_check(SETTINGS_KEY_EUI64, 0) == true) {
    settings_get(SETTINGS_KEY_EUI64, 0, (void*) &inga_cfg.eui64_addr, sizeof(inga_cfg.eui64_addr));
  } else {
    PRINTD("EUI-64 not in EEPROM\n");
    memset(inga_cfg.eui64_addr, 0, sizeof(inga_cfg.eui64_addr));
  }
#else /* !INGA_CONF_EUI64 */
  uint8_t tmp_eui64[8] = {INGA_EUI64};
  memcpy(inga_cfg.eui64_addr, tmp_eui64, 8);
#endif /* !INGA_CONF_EUI64 */

  /* PAN Addr */
#ifndef INGA_CONF_PAN_ADDR
  if (settings_check(SETTINGS_KEY_PAN_ADDR, 0) == true) {
    inga_cfg.pan_addr = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
  } else {
    inga_cfg.pan_addr = INGA_PAN_ADDR;
  }
#else /* !INGA_CONF_PAN_ADDR */
  inga_cfg.pan_adr = INGA_PAN_ADDR;
#endif /* !INGA_CONF_PAN_ADDR */


#if INGA_ADC_RANDOM
  /* If we do not have a EUI64, we generate one */
  if(eui64_is_null(inga_cfg.eui64_addr)) {
    generate_new_eui64(inga_cfg.eui64_addr);
#if INGA_WRITE_EUI64
    /* Write new settings */
    if (settings_set(SETTINGS_KEY_EUI64, inga_cfg.eui64_addr, sizeof (inga_cfg.eui64_addr)) == SETTINGS_STATUS_OK) {
      PRINTD("Wrote new IEEE Addr to EEPROM.\n");
    } else {
      PRINTD("Failed writing IEEE Addr to EEPROM.\n");
    }
#endif /* INGA_WRITE_EUI64 */
  }
#endif /* INGA_ADC_RANDOM */

  /* If we do not have a pan address, we generate one */
  if(inga_cfg.pan_addr == 0x0000) {
    inga_cfg.pan_addr = pan_addr_from_eui64(inga_cfg.eui64_addr);
  }

}
/*----------------------------------------------------------------------------*/
void
platform_radio_init(void)
{
  /* Start radio and radio receive process */
  NETSTACK_RADIO.init();

  if (eui64_is_null(inga_cfg.eui64_addr)) {
    rf230_set_pan_addr(inga_cfg.pan_id, inga_cfg.pan_addr, NULL);
  } else {
    rf230_set_pan_addr(inga_cfg.pan_id, inga_cfg.pan_addr, inga_cfg.eui64_addr);
  }

  rf230_set_channel(inga_cfg.radio_channel);
  rf230_set_txpower(inga_cfg.radio_tx_power);

  /* Initialize stack protocols */
  queuebuf_init();
  NETSTACK_RDC.init();
  NETSTACK_MAC.init();
  NETSTACK_NETWORK.init();

#if INGA_BOOTSCREEN_NETSTACK
#ifndef INGA_BOOTSCREEN
#error WOOOT?
#endif
  PRINTA_SEC("Netstack info:\n");
  PRINTA("  NET: %s\n  MAC: %s\n  RDC: %s\n",
      NETSTACK_NETWORK.name,
      NETSTACK_MAC.name,
      NETSTACK_RDC.name);
#endif /* INGA_BOOTSCREEN_NETSTACK */

#if INGA_BOOTSCREEN_RADIO
  PRINTA_SEC("Radio info:\n");
  PRINTA("  Check rate %lu Hz\n  Channel: %u\n  Power: %u",
      CLOCK_SECOND / (NETSTACK_RDC.channel_check_interval() == 0 ? 1 :
      NETSTACK_RDC.channel_check_interval()), // radio??
      rf230_get_channel(),
      rf230_get_txpower());
#if INGA_CONVERTTXPOWER
      printf(" (");
      printtxpower();
      printf(")");
#endif /* INGA_CONVERTTXPOWER */
      printf("\n");
#endif /* INGA_BOOTSCREEN_RADIO */
}

/*-------------------------Low level initialization------------------------*/
/*------Done in a subroutine to keep main routine stack usage small--------*/
void
init(void)
{
  extern void *watchdog_return_addr;
  extern uint8_t mcusr_mirror;

  /* Save the address where the watchdog occurred */
  void *wdt_addr = watchdog_return_addr;
  MCUSR = 0;

  watchdog_init();
  watchdog_start();

  /* Second rs232 port for debugging */
  rs232_init(RS232_PORT_0, INGA_USART_BAUD, USART_PARITY_NONE | USART_STOP_BITS_1 | USART_DATA_BITS_8);
  /* Redirect stdout to second port */
  rs232_redirect_stdout(RS232_PORT_0);

  /* wait here to get a chance to see boot screen. */
  _delay_ms(200);

#if INGA_BOOTSCREEN
  PRINTA("\n*******Booting %s*******\nReset reason: ", CONTIKI_VERSION_STRING);
  /* Print out reset reason */
  if (mcusr_mirror & _BV(JTRF))
    PRINTA("JTAG ");
  if (mcusr_mirror & _BV(WDRF))
    PRINTA("Watchdog ");
  if (mcusr_mirror & _BV(BORF))
    PRINTA("Brown-out ");
  if (mcusr_mirror & _BV(EXTRF))
    PRINTA("External ");
  if (mcusr_mirror & _BV(PORF))
    PRINTA("Power-on ");
  PRINTA("\n");
  if (mcusr_mirror & _BV(WDRF))
    PRINTA("Watchdog possibly occured at address %p\n", wdt_addr);
#endif /* INGA_BOOTSCREEN */

  clock_init();

#if INGA_PERIODIC_STACK
#define STACK_FREE_MARK 0x4242
  /* Simple stack pointer highwater monitor.
   * Places magic numbers in free RAM that are checked in the main loop.
   * In conjuction with INGA_PERIODIC, never-used stack will be printed
   * every INGA_PERIODIC_STACK seconds.
   */
  {
    extern uint16_t __bss_end;
    uint16_t p = (uint16_t) & __bss_end;
    do {
      *(uint16_t *) p = STACK_FREE_MARK;
      p += 10;
    } while (p < SP - 10); //don't overwrite our own stack
  }
#endif

  /* Init the random with a random (or probably different) seed.
   * This is required for several applcations, mainly in the network stack,
   * e.g. for trickle timers or the 802.15.4 packet sequence number.
   * Some layers also will ignore 802.15.4 duplicates found in a history (e.g. Contikimac)
   * causing the initial packets to be ignored after a short-cycle restart.
   */
#if INGA_ADC_RANDOM
  random_init(rng_get_uint8());
#else /* INGA_ADC_RANDOM */
  random_init(node_id);
#endif /* INGA_ADC_RANDOM */


  /* Flash initialization */
  at45db_init();

#ifdef MICRO_SD_PWR_PIN
  /* set pin for micro sd card power switch to output */
  MICRO_SD_PWR_PORT_DDR |= (1 << MICRO_SD_PWR_PIN);
#endif

  /* rtimers needed for radio cycling */
  rtimer_init();

  /* Initialize process subsystem */
  process_init();

  /* etimers must be started before ctimer_init */
  process_start(&etimer_process, NULL);

  ctimer_init();

#if (APP_SETTINGS_SET == 1)
  process_start(&settings_set_process, NULL);
#endif

#if (APP_SETTINGS_DELETE == 1)
  process_start(&settings_delete_process, NULL);
#endif    

  /* load stuff from eeprom */
  load_config();

#if INGA_BOOTSCREEN_NET
  PRINTA_SEC("WPAN Info:\n");
  PRINTA("  EUI-64:   %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n\r",
      inga_cfg.eui64_addr[0],
      inga_cfg.eui64_addr[1],
      inga_cfg.eui64_addr[2],
      inga_cfg.eui64_addr[3],
      inga_cfg.eui64_addr[4],
      inga_cfg.eui64_addr[5],
      inga_cfg.eui64_addr[6],
      inga_cfg.eui64_addr[7]);
  PRINTA("  PAN ID:   0x%04X\n", inga_cfg.pan_id);
  PRINTA("  PAN ADDR: 0x%04X\n", inga_cfg.pan_addr);
#endif /* INGA_BOOTSCREEN_NET */

  //--- Set Link address based on eui64
#if LINKADDR_SIZE == 2
  // need to invert byte order to match (short addr 0x0001 to rime addr 0.1)
  linkaddr_t inv_id = {inga_cfg.pan_addr >> 8, inga_cfg.pan_addr & 0xFF};
  linkaddr_set_node_addr(&inv_id);
#elif LINKADDR_SIZE == 8
  linkaddr_set_node_addr((linkaddr_t *) &inga_cfg.eui64_addr);
#else /* LINKADDR_SIZE */
#error LINKADDR_SIZE not supported
#endif /* LINKADDR_SIZE */

#if PLATFORM_RADIO
  // Init radio
  platform_radio_init();
#endif

#if UIP_CONF_IPV6
  // Copy EUI64 to the link local address
  memcpy(&uip_lladdr.addr, &(inga_cfg.eui64_addr), sizeof(uip_lladdr.addr));

  process_start(&tcpip_process, NULL);

#if INGA_BOOTSCREEN_NET
  PRINTA_SEC("IPv6 info:\n");
  PRINTA("  Tentative link-local IPv6 address ");
  uip_ds6_addr_t *lladdr = uip_ds6_get_link_local(-1);
  uip_debug_ipaddr_print(&lladdr->ipaddr);
  PRINTA("\n");

#if UIP_CONF_ROUTER
  PRINTA("  Routing Enabled, TCP_MSS: %u\n", UIP_TCP_MSS);
#endif
#if UIP_CONF_IPV6_RPL
  PRINTA("  RPL Enabled\n");
#endif
#endif /* INGA_BOOTSCREEN_NET */

#endif /* UIP_CONF_IPV6 */

#if INGA_BOOTSCREEN_NET
  PRINTA_SEC("Link layer info:\n");
  PRINTA("  Addr: ");
  int i;
  for (i = 0; i < sizeof(linkaddr_t) - 1; i++) {
    PRINTA("%02x.", linkaddr_node_addr.u8[i]);
  }
  PRINTA("%02x\n", linkaddr_node_addr.u8[sizeof(linkaddr_t) - 1]);
#endif /* INGA_BOOTSCREEN_NET */

  /* Start sensor init process */
  process_start(&sensors_process, NULL);

#if INGA_BOOTSCREEN_SENSORS
  PRINTA_SEC("Sensors:\n");
  CHECK_SENSOR("Button", button_sensor);
  CHECK_SENSOR("Accelerometer", acc_sensor);
  CHECK_SENSOR("Gyroscope", gyro_sensor);
  CHECK_SENSOR("Magnetometer", mag_sensor);
  CHECK_SENSOR("Pressure", pressure_sensor);
  CHECK_SENSOR("Battery", battery_sensor);
#endif /* INGA_BOOTSCREEN_SENSORS */

#if INGA_BOOTSCREEN
  PRINTA("\n******* Online *******\n\n");
#endif
}
/*---------------------------------------------------------------------------*/
#if INGA_PERIODIC
static void
periodic_prints(void)
{
  static uint32_t clocktime;

  if (clocktime != clock_seconds()) {
    clocktime = clock_seconds();

#if INGA_PERIODIC_STAMPS
    /* Print time stamps. */
    if ((clocktime % INGA_PERIODIC_STAMPS) == 0) {
#if ENERGEST_CONF_ON
#include "lib/print-stats.h"
      print_stats();
#elif RADIOSTATS
      extern volatile unsigned long radioontime;
      PRINTF("%u(%u)s\n", clocktime, radioontime);
#else /* RADIOSTATS */
      PRINTF("%us\n", clocktime);
#endif /* RADIOSTATS */
    }
#endif /* INGA_PERIODIC_STAMPS */

#if INGA_PERIODIC_ROUTES
    if ((clocktime % INGA_PERIODIC_ROUTES) == 2) {

      extern uip_ds6_netif_t uip_ds6_if;
      uint8_t i, any;
      uip_ds6_nbr_t *nbr;

      PRINTA("\nAddresses [%u max]\n", UIP_DS6_ADDR_NB);
      for (i = 0; i < UIP_DS6_ADDR_NB; i++) {
        if (uip_ds6_if.addr_list[i].isused) {
          PRINTA("  ");
          uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
          PRINTA("\n");
        }
      }

      PRINTA("\nNeighbors [%u max]\n",NBR_TABLE_MAX_NEIGHBORS);
      any = 0;
      for(nbr = nbr_table_head(ds6_neighbors);
          nbr != NULL;
          nbr = nbr_table_next(ds6_neighbors, nbr)) {
        PRINTA("  ");
        uip_debug_ipaddr_print(&nbr->ipaddr);
        PRINTA(" lladdr ");
        uip_debug_lladdr_print(uip_ds6_nbr_get_ll(nbr));
        if (nbr->isrouter) PRINTA(" router");
        switch (nbr->state) {
          case NBR_INCOMPLETE:
            PRINTA(" INCOMPLETE");
            break;
          case NBR_REACHABLE:
            PRINTA(" REACHABLE");
            break;
          case NBR_STALE:
            PRINTA(" STALE");
            break;
          case NBR_DELAY:
            PRINTA(" DELAY");
            break;
          case NBR_PROBE:
            PRINTA(" PROBE");
            break;
        }
        PRINTA("\n");
        any = 1;
      }
      if (!any) PRINTA("  <none>\n");

      PRINTA("\nRoutes [%u max]\n",UIP_DS6_ROUTE_NB);
      uip_ds6_route_t *r;
      any = 0;
      for(r = uip_ds6_route_head();
          r != NULL;
          r = uip_ds6_route_next(r)) {
        PRINTA("  ");
        uip_debug_ipaddr_print(&r->ipaddr);
        PRINTA("/%u (via ", r->length);
        uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
        PRINTA(") %lus\n", r->state.lifetime);
        any = 1;
      }
      if (!any) PRINTA("  <none>\n");
      PRINTA("\n---------\n");
    }
#endif /* INGA_PERIODIC_ROUTES */

#if INGA_PERIODIC_STACK
    /* Checks for highest address with STACK_FREE_MARKs in RAM */
    if ((clocktime % INGA_PERIODIC_STACK) == 3) {
      extern uint16_t __bss_end;
      uint16_t p = (uint16_t) & __bss_end;
      do {
        if (*(uint16_t *) p != STACK_FREE_MARK) {
          PRINTF("Never-used stack > %d bytes\n", p - (uint16_t) & __bss_end);
          break;
        }
        p += 10;
      } while (p < RAMEND - 10);
    }
#endif /* INGA_PERIODIC_STACK */
  }
}
#endif /* INGA_PERIODIC */
/*-------------------------------------------------------------------------*/
/*------------------------- Main Scheduler loop----------------------------*/
/*-------------------------------------------------------------------------*/

// setup sensors
SENSORS(&button_sensor, &acc_sensor, &gyro_sensor, &pressure_sensor, &battery_sensor, &mag_sensor);

int
main(void)
{
  init();

  /* Autostart other processes */
  autostart_start(autostart_processes);

  while (1) {
    process_run();
    watchdog_periodic();

#if DEBUGFLOWSIZE
    if (debugflowsize) {
      debugflow[debugflowsize] = 0;
      PRINTF("%s", debugflow);
      debugflowsize = 0;
    }
#endif

    watchdog_periodic();

#if INGA_PERIODIC
    periodic_prints();
#endif /* INGA_PERIODIC */

  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* implements sys/log interface */
void
log_message(char *m1, char *m2)
{
  PRINTF("%s%s\n", m1, m2);
}

