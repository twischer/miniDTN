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
	#ifdef NODE_ID
		printf("new node id: %X\n", (uint16_t) NODE_ID);
		
		if(settings_set_uint16(SETTINGS_KEY_PAN_ADDR, (uint16_t) NODE_ID) == SETTINGS_STATUS_OK)
		{
			uint16_t settings_nodeid = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
			printf("[APP.nodeid-burn] New Node ID: %X\n", settings_nodeid);
		}
		else
		{
			printf("[APP.nodeid-burn] Error: Error while writing to EEPROM\n");
		}
		
	#else
		printf("[APP.nodeid-burn] Error: No NodeID found. Aborting...\n");
	#endif
	
	process_exit(&settings_set_process);
	
	PROCESS_END();
}
