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
#define ROUTING_MAX_TRIES	 	 3
#define ROUTING_TRY_TIMEOUT		 6

/** struct to store the bundels to be routed */
struct routing_pack_list_t {
	/** number of nodes this bundle was sent to */
	uint8_t send_to;

	/** 1 if bundle is in processing */
	uint8_t action;

	/** addresses of nodes this bundle was sent to */
	rimeaddr_t dest[ROUTING_NEI_MEM];

	/** address of the last node this bundle was attempted to send to */
	rimeaddr_t last_node;

	/** number of tries to send this bundle to the 'last_node' peer */
	uint8_t last_counter;
};

process_event_t dtn_bundle_resubmission_event;

/** the route_t struct is used to inform the network interface which bundle should be transmitted to whicht node*/
struct route_t	{
	/** address of the next hop node */
	rimeaddr_t dest;
	/** bundle_num of the bundle */
	uint16_t bundle_num;
};

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
	void (* sent)(struct route_t *route,int status, int num_tx);
	void (* delete_list)(void);
	/** function to resubmit bundles currently in storage */
	void (* resubmit_bundles)(uint16_t);
};
extern const struct routing_driver ROUTING;

#endif
/** @} */
/** @} */
