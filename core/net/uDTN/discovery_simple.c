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

#ifdef DTN_DISCOVERY_SIMPLE_CONF_PS
// PS - Phase Shift Optimization
#include "discovery_scheduler.h"
#endif /* DTN_DISCOVERY_SIMPLE_CONF_PS */

#ifdef DTN_DISCOVERY_SIMPLE_CONF_FC
// FC - Flow Control
#include "storage.h"
#endif /* DTN_DISCOVERY_SIMPLE_CONF_FC */


#define DISCOVERY_NEIGHBOUR_CACHE	3


LIST(neighbour_list);
MEMB(neighbour_mem, struct discovery_neighbour_list_entry, DISCOVERY_NEIGHBOUR_CACHE);


static uint8_t discovery_simple_enabled = 0;

#ifdef DTN_DISCOVERY_SIMPLE_CONF_PS
static uint8_t discovery_simple_schedule_index = 0;
#endif  /* DTN_DISCOVERY_SIMPLE_CONF_PS */

void discovery_simple_send_discover();

#ifdef DTN_DISCOVERY_SIMPLE_CONF_AC
// AC -- Active. Send additional beacons during discovery phase
// Intra Phase Timer
static struct ctimer discovery_simple_ipt;
#endif /* DTN_DISCOVERY_SIMPLE_CONF_AC */


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
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "received from %u:%u", source->u8[1], source->u8[0]);

  if (!discovery_simple_is_neighbour(source)) {

#ifdef DTN_DISCOVERY_SIMPLE_CONF_TH
    // TH -- throttle discovery when neigbour found
    //discovery_simple_enabled = 0;
#endif /* DTN_DISCOVERY_SIMPLE_CONF_TH */


#ifdef DTN_DISCO_EVAL
    static uint32_t c = 0;
    printf("R %lu %lu %u\n", c++, clock_seconds(), source->u8[0]);
#endif

    struct discovery_neighbour_list_entry * entry;
    entry = memb_alloc(&neighbour_mem);

    memset(entry, 0, sizeof(struct discovery_neighbour_list_entry));

    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Saving neighbour %u.%u", source->u8[0], source->u8[1]);
    rimeaddr_copy(&(entry->neighbour), source);
    list_add(neighbour_list, entry);

#ifdef DTN_DISCOVERY_SIMPLE_CONF_PS
    // PS - Phase Shift Optimization
    uint32_t * neigh_id = (uint32_t *) payload;
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Schedule Index of neighbour %lu: %u      I am %lu", *neigh_id, payload[4], dtn_node_id);
    if (*neigh_id > dtn_node_id) {
      DISCOVERY_SCHEDULER.set_schedule_index(payload[4]);
    }
#endif /* DTN_DISCOVERY_SIMPLE_CONF_PS */

#ifdef DTN_DISCOVERY_SIMPLE_CONF_PR
    // Respond with discovery packet
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Responding to neighbour %u.%u", source->u8[0], source->u8[1]);
    discovery_simple_send_discover();
#endif /* DTN_DISCOVERY_SIMPLE_CONF_PR */
  }

  // We have found a new neighbour, now go and notify the agent
  process_post(&agent_process, dtn_beacon_event, source);
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

#ifdef DTN_DISCOVERY_SIMPLE_CONF_FC
  // FC -- Flow Control. Only send discover when we have bundle space left
  if (BUNDLE_STORAGE.free_space(NULL) < 0) return;
#endif /* DTN_DISCOVERY_SIMPLE_CONF_FC */

#ifdef DTN_DISCOVERY_SIMPLE_CONF_TH
  // TH -- throttle discovery when neigbour found
  if (list_length(neighbour_list)) return;
#endif /* DTN_DISCOVERY_SIMPLE_CONF_FC */

  if (discovery_simple_enabled) {
    rimeaddr_t br_dest = { { 0, 0 } };
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "send discover..");

#ifdef DTN_DISCOVERY_SIMPLE_CONF_PS
    // PS -- Phase Shift Optimization, send disco schedule index and node id
    uint8_t payload[5] = {0, 0, 0, 0, discovery_simple_schedule_index};
    memcpy(payload, &dtn_node_id, 4);
    convergence_layer_send_discovery(payload, 5, &br_dest);
#else
    //convergence_layer_send_discovery((uint8_t *) "DTN_DISCOVERY", 13, &br_dest);
    convergence_layer_send_discovery((uint8_t *) "", 0, &br_dest);
#endif /* DTN_DISCOVERY_SIMPLE_CONF_PS */
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

#ifdef DTN_DISCOVERY_SIMPLE_CONF_AC
void discovery_simple__send_helper(void * ptr) {
  ctimer_reset(&discovery_simple_ipt);
  discovery_simple_send_discover();
}
#endif /* DTN_DISCOVERY_SIMPLE_CONF_AC */

void discovery_simple_start(clock_time_t duration, uint8_t index) {
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Start of discovery phase.");
  discovery_simple_enabled = 1;
  discovery_simple_send_discover();

#ifdef DTN_DISCOVERY_SIMPLE_CONF_PS
  discovery_simple_schedule_index = index;
#endif /* DTN_DISCOVERY_SIMPLE_CONF_PS */

#ifdef DTN_DISCOVERY_SIMPLE_CONF_AC
  ctimer_set(&discovery_simple_ipt, 0.5 * duration, discovery_simple__send_helper, NULL);
#endif /* DTN_DISCOVERY_SIMPLE_CONF_AC */
}

void discovery_simple_stop() {
#ifdef DTN_DISCOVERY_SIMPLE_CONF_AC
  ctimer_stop(&discovery_simple_ipt);
#endif /* DTN_DISCOVERY_SIMPLE_CONF_AC */

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
