#ifndef __ROUTING_H__
#define __ROUTING_H__

#include <stdlib.h>
#include <stdio.h>
#include "net/dtn/bundle.h"
#include "routing.h"
struct routing_driver {
	char *name;

	void (* init)(void);
	void (* new_neighbor)(rimeaddr_t *dest);
	void (* new_bundle)(uint16_t bundle_num);
	void (* del_bundle)(uint16_t bundle_num);
	void (* sent)(uint16_t bundle_num,int status, int num_tx);
};
extern const struct routing_driver ROUTING;


struct route_t	{
	uint8_t buf;
	uint16_t bundle_num;
	rimeaddr_t dest;
};
#endif
