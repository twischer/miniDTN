/**
 * \addtogroup discovery
 * @{
 */

/**
 * \defgroup discovery_simple Simple discovery module
 *
 * @{
 */

/**
 * \file
 * \brief Simple discovery module
 *
 * \author André Frambach <frambach@ibr.cs.tu-bs.de>
 */
#include <string.h>

#include "lib/logging.h"

#include "dtn_network.h"
#include "agent.h"

#include "discovery.h"

#define DISCOVERY_NEIGHBOUR_CACHE	3


LIST(neighbour_list);
MEMB(neighbour_mem, struct discovery_neighbour_list_entry, DISCOVERY_NEIGHBOUR_CACHE);


static uint8_t discovery_simple_enabled = 0;

void discovery_simple_send_discover();


bool discovery_simple_init() {
	// Initialize the neighbour list
	list_init(neighbour_list);

	// Initialize the neighbour memory block
	memb_init(&neighbour_mem);

	return true;
}

uint8_t discovery_simple_is_neighbour(linkaddr_t * dest) {
	struct discovery_neighbour_list_entry * entry;

	for (entry = list_head(neighbour_list); entry != NULL ; entry = entry->next) {
		if (linkaddr_cmp(&(entry->neighbour), dest)) {
			return 1;
		}
	}

	return 0;
}

void discovery_simple_enable() {
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Enabled.");
	discovery_simple_enabled = 1;
}

void discovery_simple_disable() {
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Disabled.");
	discovery_simple_enabled = 0;
}

void discovery_simple_receive(linkaddr_t * source, uint8_t * payload, uint8_t length) {
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "received from %u:%u", source->u8[1], source->u8[0]);

	if (!discovery_simple_is_neighbour(source)) {
		struct discovery_neighbour_list_entry * entry;
		entry = memb_alloc(&neighbour_mem);

		memset(entry, 0, sizeof(struct discovery_neighbour_list_entry));

		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Saving neighbor %u.%u", source->u8[0], source->u8[1]);
		linkaddr_copy(&(entry->neighbour), source);
		list_add(neighbour_list, entry);

		// Respond with discovery packet
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Responding to neighbor %u.%u", source->u8[0], source->u8[1]);
		discovery_simple_send_discover();
	}

	// We have found a new neighbor, now go and notify the agent
//	process_post(&agent_process, dtn_beacon_event, source);
	const event_container_t event = {
		.event = dtn_beacon_event,
		.linkaddr = source
	};
	agent_send_event(&event);
}

void discovery_simple_alive(linkaddr_t * neighbour) {
	//LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Alive called on %u.%u", neighbour->u8[0], neighbour->u8[1]);
}

void discovery_simple_dead(linkaddr_t * neighbour) {
	struct discovery_neighbour_list_entry * entry;

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Neighbor %u.%u disappeared", neighbour->u8[0], neighbour->u8[1]);

	for (entry = list_head(neighbour_list); entry != NULL ; entry = entry->next) {
		if (linkaddr_cmp(&(entry->neighbour), neighbour)) {
			memb_free(&neighbour_mem, entry);
			list_remove(neighbour_list, entry);

			break;
		}
	}
}

void discovery_simple_send_discover() {
	if (!discovery_simple_enabled) {
		return;
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "send discover..");

	convergence_layers_send_discovery((uint8_t *) "", 0);
}

uint8_t discovery_simple_discover(linkaddr_t * dest) {
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "someone/agent asks to discover %u.%u", dest->u8[0], dest->u8[1]);

	// Check, if we already know this neighbour
	if (discovery_simple_is_neighbour(dest)) {
		return 1;
	}

	discovery_simple_send_discover();

	return 0;
}

struct discovery_neighbour_list_entry * discovery_simple_neighbours() {
	return list_head(neighbour_list);
}

void discovery_simple_stop_pending() {
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Stop pending.");
}

void discovery_simple_start(clock_time_t duration, uint8_t index) {
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Start of discovery phase.");
	discovery_simple_enabled = 1;
	discovery_simple_send_discover();
}

void discovery_simple_stop() {
	discovery_simple_send_discover();
	discovery_simple_enabled = 0;
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "End of discovery phase.");
}

void discovery_simple_clear() {
	struct discovery_neighbour_list_entry * entry;
	int changed = 1;

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Clearing neighbour list");

	/* Funktioniert das? */
	while(changed) {
		changed = 0;

		for (entry = list_head(neighbour_list); entry != NULL ; entry = entry->next) {
			convergence_layer_neighbour_down(&entry->neighbour);
			list_remove(neighbour_list, entry);
			memb_free(&neighbour_mem, entry);
			changed = 1;
			break;
		}
	}
}

const struct discovery_driver discovery_simple = {
		.name         = "SIMPLE DISCOVERY",
		.init         = discovery_simple_init,
		.is_neighbour = discovery_simple_is_neighbour,
		.enable       = discovery_simple_enable,
		.disable      = discovery_simple_disable,
		.receive      = discovery_simple_receive,
		.alive        = discovery_simple_alive,
		.dead         = discovery_simple_dead,
		.discover     = discovery_simple_discover,
		.neighbours   = discovery_simple_neighbours,
		.stop_pending = discovery_simple_stop_pending,
		.start        = discovery_simple_start,
		.stop         = discovery_simple_stop,
		.clear        = discovery_simple_clear,
};

/** @} */
/** @} */
