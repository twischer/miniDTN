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
 * \brief Simple discovery module, with active extension
 *
 * \author André Frambach <frambach@ibr.cs.tu-bs.de>
 */
#include <string.h>

#include "logging.h"

#include "dtn_network.h"
#include "agent.h"

#include "discovery.h"
#include "discovery_scheduler.h"

#define DISCOVERY_PROMPT_RESPOND
#define DISCOVERY_NEIGHBOUR_CACHE	3

LIST(neighbour_list);
MEMB(neighbour_mem, struct discovery_neighbour_list_entry, DISCOVERY_NEIGHBOUR_CACHE);

static uint8_t discovery_simple_active_enabled = 0;
static uint8_t sched_index = 0;

void discovery_simple_active_send_discover();

// Intra Phase Timer
static struct ctimer ipt;

void discovery_simple_active_init() {
  // Initialize the neighbour list
  list_init(neighbour_list);

  // Initialize the neighbour memory block
  memb_init(&neighbour_mem);
}

uint8_t discovery_simple_active_is_neighbour(rimeaddr_t * dest) {
  struct discovery_neighbour_list_entry * entry;

  for (entry = list_head(neighbour_list); entry != NULL ; entry = entry->next) {
    if (rimeaddr_cmp(&(entry->neighbour), dest)) {
      return 1;
    }
  }

  return 0;
}

void discovery_simple_active_enable() {
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Enabled.");
  discovery_simple_active_enabled = 1;
}

void discovery_simple_active_disable() {
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Disabled.");
  discovery_simple_active_enabled = 0;
}

void discovery_simple_active_receive(rimeaddr_t * source, uint8_t * payload, uint8_t length) {
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "received from %u:%u", source->u8[1], source->u8[0]);

  if (!discovery_simple_active_is_neighbour(source)) {

#ifdef DTN_DISCO_EVAL
    static uint32_t c = 0;
    printf("R %lu %lu\n", c++, clock_seconds());
#endif

    struct discovery_neighbour_list_entry * entry;
    entry = memb_alloc(&neighbour_mem);

    memset(entry, 0, sizeof(struct discovery_neighbour_list_entry));

    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Saving neighbour %u.%u", source->u8[0], source->u8[1]);
    rimeaddr_copy(&(entry->neighbour), source);
    list_add(neighbour_list, entry);


    uint32_t * neigh_id = (uint32_t *) payload;
    //printf("Schedule Index of neighbour %lu: %u      I AM %lu\n", *neigh_id, payload[4], dtn_node_id);
    if (*neigh_id > dtn_node_id) {
      DISCOVERY_SCHEDULER.set_schedule_index(payload[4]);
    }

#ifdef DISCOVERY_PROMPT_RESPOND
    // Respond with discovery packet
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Responding to neighbour %u.%u", source->u8[0], source->u8[1]);
    discovery_simple_active_send_discover();
#endif /* DISCOVERY_PROMPT_RESPOND */
  }

  // We have found a new neighbour, now go and notify the agent
  process_post(&agent_process, dtn_beacon_event, source);
}

void discovery_simple_active_alive(rimeaddr_t * neighbour) {
  //LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Alive called on %u.%u", neighbour->u8[0], neighbour->u8[1]);
}

void discovery_simple_active_dead(rimeaddr_t * neighbour) {
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

void discovery_simple_active_send_discover() {
  if (discovery_simple_active_enabled) {
    rimeaddr_t br_dest = { { 0, 0 } };
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "send discover..");

    //convergence_layer_send_discovery((uint8_t *) "DTN_DISCOVERY", 13, &br_dest);

    uint8_t payload[5] = {0, 0, 0, 0, sched_index};
    memcpy(payload, &dtn_node_id, 4);
    convergence_layer_send_discovery(payload, 5, &br_dest);

    //static uint32_t c = 0;
    //printf("D %lu\n", c++);
  } else {
    LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "send discover.. but disabled");
  }
}

uint8_t discovery_simple_active_discover(rimeaddr_t * dest) {
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "someone/agent asks to discover %u.%u", dest->u8[0], dest->u8[1]);

  // Check, if we already know this neighbour
  if (discovery_simple_active_is_neighbour(dest)) {
    return 1;
  }

  discovery_simple_active_send_discover();

  return 0;
}

struct discovery_neighbour_list_entry * discovery_simple_active_neighbours() {
  return list_head(neighbour_list);
}

void discovery_simple_active_stop_pending() {
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Stop pending.");
}

void discovery_simple_active__send_helper(void * ptr) {
  //ctimer_reset(&ipt);
  discovery_simple_active_send_discover();
}

void discovery_simple_active_start(clock_time_t duration, uint8_t index) {
  sched_index = index;
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Start of discovery phase.");
  discovery_simple_active_enabled = 1;
  discovery_simple_active_send_discover();

  ctimer_set(&ipt, 0.5 * duration, discovery_simple_active__send_helper, NULL);
}

void discovery_simple_active_stop() {
  ctimer_stop(&ipt);
  discovery_simple_active_send_discover();
  discovery_simple_active_enabled = 0;
  LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "End of discovery phase.");
}

void discovery_simple_active_clear() {
  struct discovery_neighbour_list_entry * entry;

   LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Clearing neighbour list");

   for (entry = list_head(neighbour_list); entry != NULL ; entry = entry->next) {
     memb_free(&neighbour_mem, entry);
     list_remove(neighbour_list, entry);
   }
}

const struct discovery_driver discovery_simple_active = {
    .name         = "SIMPLE DISCOVERY",
    .init         = discovery_simple_active_init,
    .is_neighbour = discovery_simple_active_is_neighbour,
    .enable       = discovery_simple_active_enable,
    .disable      = discovery_simple_active_disable,
    .receive      = discovery_simple_active_receive,
    .alive        = discovery_simple_active_alive,
    .dead         = discovery_simple_active_dead,
    .discover     = discovery_simple_active_discover,
    .neighbours   = discovery_simple_active_neighbours,
    .stop_pending = discovery_simple_active_stop_pending,
    .start        = discovery_simple_active_start,
    .stop         = discovery_simple_active_stop,
    .clear        = discovery_simple_active_clear,
};

/** @} */
/** @} */
