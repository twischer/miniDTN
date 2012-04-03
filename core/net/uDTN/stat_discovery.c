#include "custody-signal.h"
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
#include "dev/leds.h"
#include "storage.h"


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void s_dis_init()
{

}

uint8_t s_dis_neighbour(rimeaddr_t * dest)
{
	return 1;
}

void s_dis_enable()
{

}

void s_dis_disable()
{

}

void s_dis_receive(rimeaddr_t * source, uint8_t * payload, uint8_t length)
{

}

void s_dis_alive(rimeaddr_t * source)
{

}

uint8_t d_dis_discover(rimeaddr_t * dest)
{
	return 1;
}

struct discovery_neighbour_list_entry * s_dis_neighbours()
{
	return NULL;
}

void s_dis_stop_pending()
{

}

const struct discovery_driver s_discovery ={
	"S_DISCOVERY",
	s_dis_init,
	s_dis_neighbour,
	s_dis_enable,
	s_dis_disable,
	s_dis_receive,
	s_dis_alive,
	d_dis_discover,
	s_dis_neighbours,
	s_dis_stop_pending,
};

