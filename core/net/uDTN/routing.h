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
 * \brief this file defines the interface for routing modules
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef __ROUTING_H__
#define __ROUTING_H__

#include <stdlib.h>
#include <stdio.h>

#include "contiki.h"
#include "net/linkaddr.h"
#include "lib/memb.h"
#include "list.h"

#include "bundle.h"
#include "convergence_layer.h"

/**
 * Which routing driver are we going to use?
 */
#ifdef CONF_ROUTING
#define ROUTING CONF_ROUTING
#else
#define ROUTING routing_flooding
#endif

/**
 * For each bundle, how many neighbours to which
 * the bundle has been sent before should be stored?
 */
#ifdef CONF_ROUTING_NEIGHBOURS
#define ROUTING_NEI_MEM CONF_ROUTING_NEIGHBOURS
#else
#define ROUTING_NEI_MEM 2
#endif

/**
 * Routing bundle Flags
 */
#define ROUTING_FLAG_IN_DELIVERY	0x01
#define ROUTING_FLAG_LOCAL			0x02
#define ROUTING_FLAG_FORWARD		0x04
#define ROUTING_FLAG_IN_TRANSIT		0x08

/**
 * Routing sent status reports
 */
#define ROUTING_STATUS_OK			0x01
#define ROUTING_STATUS_FAIL			0x02
#define ROUTING_STATUS_NACK			0x04
#define ROUTING_STATUS_ERROR		0x08
#define ROUTING_STATUS_TEMP_NACK	0x10

PROCESS_NAME(routing_process);

/** the route_t struct is used to inform the network interface which bundle should be transmitted to whicht node*/
struct route_t	{
	/** address of the next hop node */
	linkaddr_t dest;

	/** bundle_num of the bundle */
	uint32_t bundle_num;
};

/** Interface for routing modules */
struct routing_driver {
	char *name;
	/** module init, called by agent at startup*/
	void (* init)(void);
	/** informs the module about a new neighbor */
	void (* new_neighbor)(linkaddr_t * dest);
	/** informs the module about a new bundel */
	int (* new_bundle)(uint32_t * bundle_num);
	/** delete bundle form routing list */
	void (* del_bundle)(uint32_t bundle_num);
	/** callback function is called by convergence layer */
	void (* sent)(struct transmit_ticket_t * ticket, uint8_t status);
	/** function to resubmit bundles currently in storage */
	void (* resubmit_bundles)();
	/** notify storage, that bundle has been delivered locally */
	void (* locally_delivered)(struct mmem * bundlemem);
};
extern const struct routing_driver ROUTING;

#endif
/** @} */
/** @} */
