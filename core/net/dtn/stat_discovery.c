#include "net/dtn/custody-signal.h"
#include "dtn_config.h"
#include "dtn-network.h"
#include "clock.h"
#include "net/netstack.h"
#include "net/packetbuf.h" 
#include "net/rime/rimeaddr.h"

#include "agent.h"
#include "status-report.h"
#include "dtn-network.h" 
#include "discovery.h"


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif



static rimeaddr_t dest;
void s_dis_send(struct bundle_t *bundle)
{
	dest.u8[0]=12;
	dest.u8[1]=0;
	process_post(&agent_process, dtn_beacon_event, &dest);
	return;
}

uint8_t s_dis_is_beacon(uint8_t *msg)
{
			return 0;
}

uint8_t s_dis_is_discover(uint8_t *msg)
{
			return 0;
}
const struct discovery_driver s_discovery ={
	"S_DISCOVERY",
	s_dis_send,
	s_dis_is_beacon,
	s_dis_is_discover,
};

