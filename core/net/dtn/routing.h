#ifndef __ROUTING_H__
#define __ROUTING_H__

#include <stdlib.h>
#include <stdio.h>
#include "bundle.h"
#include "contiki.h"
#include "net/rime/rimeaddr.h"
#include "lib/memb.h"
#include "list.h"

#define ROUTING_ROUTE_MAX_MEM 10
struct routing_driver {
	char *name;
	void (* init)(void);
	void (* new_neighbor)(rimeaddr_t *dest);
	int (* new_bundle)(uint16_t bundle_num);
	void (* del_bundle)(uint16_t bundle_num);
	void (* sent)(uint16_t bundle_num,int status, int num_tx);
};
extern const struct routing_driver ROUTING;


struct route_t	{
	struct route_t *next;
	rimeaddr_t dest;
	uint16_t bundle_num;
	rimeaddr_t dest;
};
MEMB(route_mem, struct route_t, ROUTING_ROUTE_MAX_MEM);
#endif
