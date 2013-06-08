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

#include "logging.h"

#include "dtn_network.h"
#include "agent.h"

#include "discovery.h"

#define DISCOVERY_PROMPT_RESPOND
#define DISCOVERY_NEIGHBOUR_CACHE	3

LIST(neighbour_list);
MEMB(neighbour_mem, struct discovery_neighbour_list_entry, DISCOVERY_NEIGHBOUR_CACHE);


static uint8_t discovery_simple_enabled = 0;

void discovery_simple_send_discover();

void discovery_simple_init() {
  // Initialize the neighbour list
  list_init(neighbour_list);

  // Initialize the neighbour memory block
  memb_init(&neighbour_mem);
}

uint8_t discovery_simple_is_neighbour(rimeaddr_t * dest) {
  struct discovery_neighbour_list_entry * entry;

  for (entry = list_head(neighbour_list); entry != NULL ; entry = entry->next) {
    if (rimeaddr_cmp(&(entry->neighbour), dest)) {
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

void discovery_simple_receive(rimeaddr_t * source, uint8_t * payload, uint8_t length) {
  //if (discovery_simple_enabled) {
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "received from %u:%u", source->u8[1], source->u8[0]);

    if (!discovery_simple_is_neighbour(source)) {
      static uint32_t c = 0;
      printf("R %lu\n", c++);

      struct discovery_neighbour_list_entry * entry;
      entry = memb_alloc(&neighbour_mem);

      memset(entry, 0, sizeof(struct discovery_neighbour_list_entry));

      LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Saving neighbour %u.%u", source->u8[0], source->u8[1]);
      rimeaddr_copy(&(entry->neighbour), source);
      list_add(neighbour_list, entry);

#ifdef DISCOVERY_PROMPT_RESPOND
      // Respond with discovery packet
      LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Responding to neighbour %u.%u", source->u8[0], source->u8[1]);
      discovery_simple_send_discover();
#endif /* DISCOVERY_PROMPT_RESPOND */
    }

    // We have found a new neighbour, now go and notify the agent
    process_post(&agent_process, dtn_beacon_event, source);
  /*} else {
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "received from %u:%u but disabled", source->u8[1], source->u8[0]);
  }*/
}

void discovery_simple_alive(rimeaddr_t * neighbour) {
  //LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Alive called on %u.%u", neighbour->u8[0], neighbour->u8[1]);
}

void discovery_simple_dead(rimeaddr_t * neighbour) {
  struct discovery_neighbour_list_entry * entry;

  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Neighbour %u.%u disappeared", neighbour->u8[0], neighbour->u8[1]);

  for (entry = list_head(neighbour_list); entry != NULL ; entry = entry->next) {
    if (rimeaddr_cmp(&(entry->neighbour), neighbour)) {
      memb_free(&neighbour_mem, entry);
      list_remove(neighbour_list, entry);

      break;
    }
  }
}

void discovery_simple_send_discover() {
  if (discovery_simple_enabled) {
    rimeaddr_t br_dest = { { 0, 0 } };
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "send discover..");

    convergence_layer_send_discovery((uint8_t *) "DTN_DISCOVERY", 13, &br_dest);

    //static uint32_t c = 0;
    //printf("D %lu\n", c++);
  } else {
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "send discover.. but disabled");
  }
}

uint8_t discovery_simple_discover(rimeaddr_t * dest) {
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

void discovery_simple_start(clock_time_t duration) {
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

   LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Clearing neighbour list");

   for (entry = list_head(neighbour_list); entry != NULL ; entry = entry->next) {
     memb_free(&neighbour_mem, entry);
     list_remove(neighbour_list, entry);
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
