/**
 * \addtogroup agent
 * @{
 */

 /**
 * \defgroup routing Routing modules
 *
 * @{
 */

/**
 * \file 
 * this file defines the interface for routing modules
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
 */
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
/** Interface for routing modules */
struct routing_driver {
	char *name;
	/** module init, called by agent at startup*/
	void (* init)(void);
	/** informs the module about a new neighbor */
	void (* new_neighbor)(rimeaddr_t *dest);
	/** informs the module about a new bundel */
	int (* new_bundle)(uint16_t bundle_num);
	/** delete bundel form routing list */
	void (* del_bundle)(uint16_t bundle_num);
	/** callback funktion is called by network interface */
	void (* sent)(uint16_t bundle_num,int status, int num_tx);
	void (* delete_list)(void);
};
extern const struct routing_driver ROUTING;

/** the route_t struct is used to inform the network interface which bundle should be transmitted to whicht node*/
struct route_t	{
	struct route_t *next;
	/** address of the next hop node */
	rimeaddr_t dest;
	/** bundle_num of the bundle */
	uint16_t bundle_num;
};
/** memory for route_ts */
MEMB(route_mem, struct route_t, ROUTING_ROUTE_MAX_MEM);
#endif
/** @} */
/** @} */
