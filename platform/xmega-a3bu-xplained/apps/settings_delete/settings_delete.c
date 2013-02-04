#include "contiki.h"
#include "contiki-lib.h"
#include "settings.h"
#include <stdio.h>

#include "settings_delete.h"

PROCESS(settings_delete_process, "Burn NodeID Process");

// AUTOSTART_PROCESSES(&nodeid_burn_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(settings_delete_process, ev, data)
{
	PROCESS_BEGIN();
	
	// Delete all Settings if no value is defined
	//#if !defined(NODE_CONF_ID) && !defined(RADIO_CONF_CHANNEL) && !defined(RADIO_CONF_TX_POWER)
		settings_wipe();
	//#elif defined(NODE_CONF_ID)
	//	printf("[APP.nodeid-burn] Delete Status: %d\n", settings_delete(SETTINGS_KEY_PAN_ID, 0) == SETTINGS_STATUS_OK ? 1 : 0);
	//#endif
	
	process_exit(&settings_delete_process);
	
	PROCESS_END();
}
