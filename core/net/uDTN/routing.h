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

#define ROUTING_ROUTE_MAX_MEM 	10
#define ROUTING_NEI_MEM 	 	 2

#define ROUTING_FLAG_IN_DELIVERY	0x01
#define ROUTING_FLAG_LOCAL			0x02
#define ROUTING_FLAG_FORWARD		0x04
#define ROUTING_FLAG_IN_TRANSIT		0x08

process_event_t dtn_bundle_resubmission_event;

/** the route_t struct is used to inform the network interface which bundle should be transmitted to whicht node*/
struct route_t	{
	/** address of the next hop node */
	rimeaddr_t dest;

	/** bundle_num of the bundle */
	uint32_t bundle_num;
};

/** Interface for routing modules */
struct routing_driver {
	char *name;
	/** module init, called by agent at startup*/
	void (* init)(void);
	/** informs the module about a new neighbor */
	void (* new_neighbor)(rimeaddr_t *dest);
	/** informs the module about a new bundel */
	int (* new_bundle)(uint32_t bundle_num);
	/** delete bundle form routing list */
	void (* del_bundle)(uint32_t bundle_num);
	/** callback funktion is called by network interface */
	void (* sent)(struct route_t *route,int status, int num_tx);
	void (* delete_list)(void);
	/** function to resubmit bundles currently in storage */
	void (* resubmit_bundles)(uint8_t called_by_event);
	/** notify storage, that bundle has been delivered locally */
	void (* locally_delivered)(struct mmem * bundlemem);
};
extern const struct routing_driver ROUTING;

#endif
/** @} */
/** @} */
