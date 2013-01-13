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
		
		if(settings_set_uint16(SETTINGS_KEY_PAN_ID, (uint16_t) NODE_ID) == SETTINGS_STATUS_OK)
		{
			uint16_t settings_nodeid = settings_get_uint16(SETTINGS_KEY_PAN_ID, 0);
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
