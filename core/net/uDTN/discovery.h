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

/**
 * Which discovery driver are we going to use?
 */
#ifdef CONF_DISCOVERY
#define DISCOVERY CONF_DISCOVERY
#else
#define DISCOVERY discovery_ipnd
#endif



//PROCESS_NAME(discovery_process);

struct discovery_neighbour_list_entry {
	struct discovery_neighbour_list_entry *next;
	linkaddr_t neighbour;
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
	uint8_t (* is_neighbour)(const linkaddr_t* const dest);

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
	void (* receive)(linkaddr_t * source, uint8_t * payload, uint8_t length);
	void (* receive_ip)(const struct ip_addr* const ip, const uint8_t* const payload, const uint8_t length);

	/**
	 * Bundle from node has been received, cache this node as available
	 */
	void (* alive)(linkaddr_t * source);
	bool (* alive_ip)(const struct ip_addr* const ip, const uint16_t port);

	/**
	 * Multiple transmission attempts to this neighbour have failed
	 */
	void (* dead)(linkaddr_t * destination);

	/**
	 * Starts to discover a neighbour
	 */
	uint8_t (* discover)(const linkaddr_t* const dest);

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

#endif
/** @} */
/** @} */
