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



void s_dis_send(struct bundle_t *bundle)
{
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

