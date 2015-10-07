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

/**
 * Which discovery driver are we going to use?
 */
#ifdef CONF_DISCOVERY
#define DISCOVERY CONF_DISCOVERY
#else
#define DISCOVERY discovery_ipnd
#endif


#define ADDRESS_TYPE_FLAG_LOWPAN	(1 << 1)
#define ADDRESS_TYPE_FLAG_IPV4		(1 << 2)

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
	char *name;

	/**
	 * Initialize discovery module
	 */
	bool (* init)();

	/**
	 * Ask discovery, if this node is currently in range
	 * return 1 if yes,
	 * return 0 otherwise
	 */
	bool (* is_neighbour)(const uint32_t eid);

	/**
	 * Enable discovery module
	 */
	void (* enable)();

	/**
	 * Disable discovery module
	 */
	void (* disable)();

	/**
	 * Pass incoming discovery beacons to the discovery module
	 */
	int (* receive)(const cl_addr_t* const addr, const uint8_t* const payload, const uint8_t length);

	/**
	 * Bundle from node has been received, cache this node as available
	 */
	int (* alive)(const cl_addr_t* const neighbour);
	// TODO int (* alive_eid)(const uint32_t eid);

	/**
	 * Multiple transmission attempts to this neighbour have failed
	 */
	void (* dead)(const cl_addr_t* const neighbour);

	/**
	 * Starts to discover a neighbour
	 */
	bool (* discover)(const uint32_t eid);

	/**
	 * Returns the list of currently known neighbours
	 */
	struct discovery_neighbour_list_entry * (* neighbours)();

	/**
	 * Stops pending discoveries
	 */
	void (* stop_pending)();

	/**
	 * Start discovery phase, called by discovery scheduler
	 */
	void (* start)(clock_time_t duration, uint8_t index);

	/**
	 * Stop discovery phase, called by discovery scheduler
	 */
	void (* stop)();

	/**
	 * Clear the list of currently known neighbours
	 */
	void (* clear)();
};

extern const struct discovery_driver DISCOVERY;


static inline bool discovery_neighbour_cmp(const struct discovery_neighbour_list_entry* const entry, const cl_addr_t* const addr)
{
	if (addr->isIP) {
		/* check, if the entry contains an IP address */
		if ( (entry->addr_type & ADDRESS_TYPE_FLAG_IPV4) == 0 ) {
			return false;
		}
		if ( !ip_addr_cmp(&entry->ip, &addr->ip) ) {
			return false;
		}

		return (entry->port == addr->port);
	} else {
		/* check, if the entry contains a lowpan address */
		if ( (entry->addr_type & ADDRESS_TYPE_FLAG_LOWPAN) == 0 ) {
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

	if ( (addr_type & ADDRESS_TYPE_FLAG_LOWPAN) == ADDRESS_TYPE_FLAG_LOWPAN ) {
		addr->isIP = false;
		linkaddr_copy(&addr->lowpan, &entry->neighbour);
	} else if ( (addr_type & ADDRESS_TYPE_FLAG_IPV4) == ADDRESS_TYPE_FLAG_IPV4 ) {
		addr->isIP = true;
		ip_addr_copy(addr->ip, entry->ip);
		addr->port = entry->port;
	} else {
		/* an unkown type was used */
		return -2;
	}

	return 0;
}


#endif
/** @} */
/** @} */
