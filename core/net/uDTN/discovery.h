/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup discovery Discovery modules
 *
 * @{
 */

/**
 * \file
 * \brief defines the interface for discovery modules
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <stdbool.h>

#include "net/rime/rime.h"
#include "lwip/ip_addr.h"
#include "cl_address.h"
#include "convergence_layer_lowpan_dgram.h"
#include "convergence_layer_udp_dgram.h"

/**
 * Which discovery driver are we going to use?
 */
#ifdef CONF_DISCOVERY
#define DISCOVERY CONF_DISCOVERY
#else
#define DISCOVERY discovery_ipnd
#endif


#define CL_TYPE_FLAG_DGRAM_LOWPAN	(1 << 1)
#define CL_TYPE_FLAG_DGRAM_UDP		(1 << 2)

struct discovery_neighbour_list_entry {
	struct discovery_neighbour_list_entry *next;
	// TODO change all inheriting struct definitions of this type, too
	uint8_t addr_type;
	// TODO interpret as EID and use uint32_t, because compressed bundle header can have such big addresses
	linkaddr_t neighbour;
	ip_addr_t ip;
	uint16_t port;
};

/** interface for discovery modules */
struct discovery_driver {
	const char* const name;

	/**
	 * Initialize discovery module
	 */
	bool (* const init)();

	/**
	 * Ask discovery, if this node is currently in range
	 * return 1 if yes,
	 * return 0 otherwise
	 */
	bool (* const is_neighbour)(const uint32_t eid);

	/**
	 * Enable discovery module
	 */
	void (* const enable)();

	/**
	 * Disable discovery module
	 */
	void (* const disable)();

	/**
	 * Pass incoming discovery beacons to the discovery module
	 */
	int (* const receive)(const cl_addr_t* const addr, const uint8_t* const payload, const uint8_t length);

	/**
	 * Bundle from node has been received, cache this node as available
	 */
	int (* const alive)(const cl_addr_t* const neighbour);
	// TODO int (* alive_eid)(const uint32_t eid);

	/**
	 * Multiple transmission attempts to this neighbour have failed
	 */
	void (* const dead)(const cl_addr_t* const neighbour);

	/**
	 * Starts to discover a neighbour
	 */
	bool (* const discover)(const uint32_t eid);

	/**
	 * Returns the list of currently known neighbours
	 */
	struct discovery_neighbour_list_entry * (* const neighbours)();

	/**
	 * Stops pending discoveries
	 */
	void (* const stop_pending)();

	/**
	 * Start discovery phase, called by discovery scheduler
	 */
	void (* const start)(clock_time_t duration, uint8_t index);

	/**
	 * Stop discovery phase, called by discovery scheduler
	 */
	void (* const stop)();

	/**
	 * Clear the list of currently known neighbours
	 */
	void (* const clear)();
};

extern const struct discovery_driver DISCOVERY;


static inline bool discovery_neighbour_cmp(const struct discovery_neighbour_list_entry* const entry, const cl_addr_t* const addr)
{
	if (addr->isIP) {
		/* check, if the entry contains an IP address */
		if ( (entry->addr_type & CL_TYPE_FLAG_DGRAM_UDP) == 0 ) {
			return false;
		}
		if ( !ip_addr_cmp(&entry->ip, &addr->ip) ) {
			return false;
		}

		return (entry->port == addr->port);
	} else {
		/* check, if the entry contains a lowpan address */
		if ( (entry->addr_type & CL_TYPE_FLAG_DGRAM_LOWPAN) == 0 ) {
			return false;
		}

		return linkaddr_cmp(&entry->neighbour, &addr->lowpan);
	}

	/*
	 * should not reach this end,
	 * because of else block
	 */
	return false;
}


static inline int discovery_neighbour_to_addr(const struct discovery_neighbour_list_entry* const entry, const uint8_t addr_type, cl_addr_t* const addr)
{
	/* check, if reqested type is existing */
	if ( (entry->addr_type & addr_type) == 0 ) {
		return -1;
	}

	if ( (addr_type & CL_TYPE_FLAG_DGRAM_LOWPAN) == CL_TYPE_FLAG_DGRAM_LOWPAN ) {
		cl_addr_build_lowpan_dgram(&entry->neighbour, addr);
	} else if ( (addr_type & CL_TYPE_FLAG_DGRAM_UDP) == CL_TYPE_FLAG_DGRAM_UDP ) {
		cl_addr_build_udp_dgram(&entry->ip, entry->port, addr);
	} else {
		/* an unkown type was used */
		return -2;
	}

	return 0;
}


#endif
/** @} */
/** @} */
