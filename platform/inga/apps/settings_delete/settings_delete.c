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

#include "contiki.h"
#include "contiki-lib.h"
#include "settings.h"
#include <stdio.h>

#include "settings_delete.h"

#define PRINTF printf 

/**
 * Deletes settings depending on defines and terminates.
 */
PROCESS(settings_delete_process, "Settings delete Process");

//static void _do_delete(char* descr, settings_key_t key) {
//  uint8_t ret = settings_delete(key, 0);
//  PRINTF("[APP.settings-delete] %s delete status: %s\n", descr, ret ? "OK" : "FAILED");
//}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(settings_delete_process, ev, data)
{
  PROCESS_BEGIN();

#if (APP_SETTINGS_DELETE == 1)
  // Delete all Settings if no value is defined
#if !defined(NODE_CONF_ID) && !defined(RADIO_CONF_PAN_ID) && !defined(RADIO_CONF_CHANNEL) && !defined(RADIO_CONF_TX_POWER) && !defined(NODE_CONF_EUI64)
  PRINTF("Wiping settings...");
  settings_wipe();
  PRINTF("done.\n");
#else /* !defined(NODE_CONF_ID) && !defined(RADIO_CONF_CHANNEL) && !defined(RADIO_CONF_TX_POWER) */
// NOTE: currently deleting single items is disabled as the library does not provide it!
// TODO: Check Roberts implementation for delete function
#error Settings manager does not support deleting single items yet. Try wipe instead.
#ifdef NODE_CONF_ID
  PRINTF("[APP.settings-delete] node id delete status: %s\n", settings_delete(SETTINGS_KEY_PAN_ADDR, 0) == SETTINGS_STATUS_OK ? "OK" : "FAILED");
  //_do_delete("node id", SETTINGS_KEY_PAN_ADDR);
#endif
#ifdef RADIO_CONF_PAN_ID
  PRINTF("[APP.settings-delete] pan id delete status: %d\n", settings_delete(SETTINGS_KEY_PAN_ID, 0) == SETTINGS_STATUS_OK ? 1 : 0);
#endif
#ifdef RADIO_CONF_CHANNEL
  PRINTF("[APP.settings-delete] channel delete status: %d\n", settings_delete(SETTINGS_KEY_CHANNEL, 0) == SETTINGS_STATUS_OK ? 1 : 0);
#endif
#ifdef RADIO_CONF_TX_POWER
  PRINTF("[APP.settings-delete] tx power delete status: %d\n", settings_delete(SETTINGS_KEY_TXPOWER, 0) == SETTINGS_STATUS_OK ? 1 : 0);
#endif
#ifdef NODE_CONF_EUI64
  PRINTF("[APP.settings-delete] EUI64 delete status: %d\n", settings_delete(SETTINGS_KEY_EUI64, 0) == SETTINGS_STATUS_OK ? 1 : 0);
#endif
#endif /* !defined(NODE_CONF_ID) && !defined(RADIO_CONF_PAN_ID) && !defined(RADIO_CONF_CHANNEL) && !defined(RADIO_CONF_TX_POWER) && !defined(NODE_CONF_EUI64) */

#endif /* (APP_SETTINGS_DELETE == 1) */

  PROCESS_END();
}
