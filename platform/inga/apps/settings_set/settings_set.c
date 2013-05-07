/*
 * Copyright (c) 2013, TU Braunschweig
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
 * \file
 *        Settings Set Application
 *
 * \author
 *        Robert Hartung
 *        Enrico Joerns <e.joerns@tu-bs.de>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "settings.h"
#include <stdio.h>

#include "settings_set.h"

PROCESS(settings_set_process, "Settings Set Process");

// AUTOSTART_PROCESSES(&nodeid_burn_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(settings_set_process, ev, data)
{
  PROCESS_BEGIN();

  // define comes from our contiki-conf.h based on NODE_CONF_ID
#ifdef NODE_CONF_ID
  if (settings_set_uint16(SETTINGS_KEY_PAN_ADDR, (uint16_t) NODE_ID) == SETTINGS_STATUS_OK) {
    uint16_t settings_nodeid = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
    printf("[APP.settings_set] New Node ID: %X\n", settings_nodeid);
  } else {
    printf("[APP.settings_set] Error: Error while writing NodeID to EEPROM\n");
  }
#else
  //  printf("[APP.settings_set] No NodeID found. Skipping...\n");
#endif

#ifdef RADIO_CONF_PAN_ID
  if (settings_set_uint16(SETTINGS_KEY_PAN_ID, (uint16_t) PAN_ID) == SETTINGS_STATUS_OK) {
    uint16_t settings_panid = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
    printf("[APP.settings_set] New Pan ID: %X\n", settings_panid);
  } else {
    printf("[APP.settings_set] Error: Error while writing PanID to EEPROM\n");
  }
#else
  //  printf("[APP.settings_set] No PanID found. Skipping...\n");
#endif

#ifdef RADIO_CONF_CHANNEL
  if (settings_set_uint8(SETTINGS_KEY_CHANNEL, (uint8_t) RADIO_CHANNEL) == SETTINGS_STATUS_OK) {
    uint8_t settings_channel = settings_get_uint8(SETTINGS_KEY_CHANNEL, 0);
    printf("[APP.settings_set] New channel: %X\n", settings_channel);
  } else {
    printf("[APP.settings_set] Error: Error while writing channel to EEPROM\n");
  }
#else
  //  printf("[APP.settings_set] No channel found. Skipping...\n");
#endif


#ifdef RADIO_CONF_TX_POWER
  if (settings_set_uint8(SETTINGS_KEY_TXPOWER, (uint8_t) RADIO_TX_POWER) == SETTINGS_STATUS_OK) {
    uint8_t settings_txpower = settings_get_uint8(SETTINGS_KEY_TXPOWER, 0);
    printf("[APP.settings_set] New TX power: %X\n", settings_txpower);
  } else {
    printf("[APP.settings_set] Error: Error while writing TX power to EEPROM\n");
  }
#else
  //  printf("[APP.settings_set] No TX power found. Skipping...\n");
#endif

#ifdef EUI64
  if (settings_set(SETTINGS_KEY_EUI64, EUI64, 8) == SETTINGS_STATUS_OK) {
    uint8_t settings_eui64[8];
    settings_get(SETTINGS_KEY_EUI64, 0, settings_eui64, sizeof (settings_eui64));
    printf("[APP.settings_set] New EUI64: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n\r",
            settings_eui64[0],
            settings_eui64[1],
            settings_eui64[2],
            settings_eui64[3],
            settings_eui64[4],
            settings_eui64[5],
            settings_eui64[6],
            settings_eui64[7]);
  } else {
    printf("[APP.settings_set] Error: Error while writing to EEPROM\n");
  }
#else
  //  printf("[APP.settings_set] No EUI64 found. Skipping...\n");
#endif


  process_exit(&settings_set_process);

  PROCESS_END();
}
