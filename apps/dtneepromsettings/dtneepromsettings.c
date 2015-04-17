/*
 * Copyright (c) 2014, Wolf-Bastian Pöttner.
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
 */

#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "settings.h"

#include "agent.h"

#include "dtn_apps.h"

#ifndef CONTIKI_CONF_SETTINGS_MANAGER
#error CONTIKI_CONF_SETTINGS_MANAGER is not enabled
#endif

#define SETTINGS_KEY_BUNDLE_SEQUENCE_NUMBER TCC('S','N')

struct ctimer timer;
uint32_t last_value = 0;

/*---------------------------------------------------------------------------*/
void dtneepromsettings_save(void * tmp)
{
	/* Avoid writing the EEPROM if nothing changed */
	if( agent_get_sequence_number() != last_value ) {
		settings_set_uint32(SETTINGS_KEY_BUNDLE_SEQUENCE_NUMBER, agent_get_sequence_number());
	}

	last_value = agent_get_sequence_number();

	ctimer_reset(&timer);
}
/*---------------------------------------------------------------------------*/
int dtneepromsettings_init(void)
{

	if( settings_check(SETTINGS_KEY_BUNDLE_SEQUENCE_NUMBER, 0) > 0 ) {
		/* Set the sequence number if the agent to the last seen value + 60 to compensate
		 * for some of the increments that were not saved
		 */
		agent_set_sequence_number(settings_get_uint32(SETTINGS_KEY_BUNDLE_SEQUENCE_NUMBER, 0) + 60);
	} else {
		settings_add_uint32(SETTINGS_KEY_BUNDLE_SEQUENCE_NUMBER, agent_get_sequence_number());
	}

	/* Update the EEPROM every 60s */
	ctimer_set(&timer, 60 * CLOCK_SECOND, dtneepromsettings_save, NULL);

	return 1;
}
/*---------------------------------------------------------------------------*/
const struct dtn_app dtneepromsettings = {
		.name = "uDTN Persistent EEPROM Settings",
		.init = dtneepromsettings_init,
};
/*---------------------------------------------------------------------------*/
