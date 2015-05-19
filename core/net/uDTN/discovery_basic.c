/**
 * \addtogroup discovery 
 * @{
 */

/**
 * \defgroup discovery_basic Basic discovery module
 *
 * @{
 */

/**
 * \file
 * \brief Basic discovery module
 *
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#include <string.h> // for memset

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "net/netstack.h"
#include "net/packetbuf.h" 
#include "net/linkaddr.h"
#include "lib/logging.h"

#include "dtn_network.h"
#include "agent.h"
#include "discovery_scheduler.h"

#include "discovery.h"

void discovery_basic_neighbour_found(linkaddr_t * neighbour);
void discovery_basic_refresh_neighbour(linkaddr_t * neighbour);
void discovery_basic_save_neighbour(linkaddr_t * neighbour);
void discovery_basic_stop_pending();

#define DISCOVERY_NEIGHBOUR_CACHE	3
#define DISCOVERY_NEIGHBOUR_TIMEOUT	(5 * DISCOVERY_CYCLE)
#define DISCOVERY_TRIES				5

/**
 * This "internal" struct extends the discovery_neighbour_list_entry struct with
 * more attributes for internal use
 */
struct discovery_basic_neighbour_list_entry {
	struct discovery_basic_neighbour_list_entry *next;
	linkaddr_t neighbour;
	clock_time_t timestamp;
	uint8_t active;
};

//PROCESS(discovery_process, "DISCOVERY process");
static TaskHandle_t discovery_task;
static void discovery_process(void* p);

/**
 * List and memory blocks to save information about neighbours
 */
LIST(neighbour_list);
MEMB(neighbour_mem, struct discovery_basic_neighbour_list_entry, DISCOVERY_NEIGHBOUR_CACHE);

static uint8_t discovery_status = 0;
uint8_t discovery_pending = 0;
uint8_t discovery_pending_start = 0;
static TimerHandle_t discovery_pending_timer;

static void discovery_timeout(const TimerHandle_t timer);
static void discovery_pending_timeout(const TimerHandle_t timer);

/**
 * \brief Initialize basic discovery module
 */
bool discovery_basic_init()
{
	// Enable discovery module
	discovery_status = 1;

	// Initialize the neighbour list
	list_init(neighbour_list);

	// Initialize the neighbour memory block
	memb_init(&neighbour_mem);


//	etimer_set(&discovery_timeout_timer, DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND);
	const TimerHandle_t discovery_timeout_timer = xTimerCreate("discovery timeout timer", pdMS_TO_TICKS(DISCOVERY_NEIGHBOUR_TIMEOUT * 1000),
										   pdTRUE, NULL, discovery_timeout);
	if (discovery_timeout_timer == NULL) {
		return false;
	}

	if ( !xTimerStart(discovery_timeout_timer, 0) ) {
		return false;
	}

	discovery_pending_timer = xTimerCreate("discovery pending timer", pdMS_TO_TICKS(DISCOVERY_CYCLE * 1000),
										   pdFALSE, NULL, discovery_pending_timeout);
	if (discovery_pending_timer == NULL) {
		return false;
	}

	// Start discovery process
//	process_start(&discovery_process, NULL);
	if ( !xTaskCreate(discovery_process, "DISCOVERY process", configMINIMAL_STACK_SIZE, NULL, 1, &discovery_task) ) {
		return false;
	}

	return true;
}

/** 
 * \brief Is the provided address currently listed as neighbour?
 * \param dest Address of the potential neighbour
 * \return 1 if neighbour, 0 otherwise
 */
uint8_t discovery_basic_is_neighbour(linkaddr_t * dest)
{
	struct discovery_basic_neighbour_list_entry * entry;

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = entry->next) {
		if( entry->active &&
				linkaddr_cmp(&(entry->neighbour), dest) ) {
			return 1;
		}
	}

	return 0;
}

/**
 * \brief Send out discovery to provided address
 * \param destination Address to send the discovery to
 */
void discovery_basic_send_discovery(linkaddr_t * destination)
{
	if( discovery_status == 0 ) {
		return;
	}

	linkaddr_t dest={{0,0}};
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "dtn_send_discover");

	convergence_layer_send_discovery((uint8_t *) "DTN_DISCOVERY", 13, &dest);
}

/**
 * \brief checks if incoming message is an answer to a discovery message
 * \param msg pointer to the received message
 * \return 1 if msg is an answer or 0 if not
 */
uint8_t discovery_basic_is_beacon(uint8_t * msg)
{
	uint8_t test[8]="DTN_HERE";
	uint8_t i;
	for (i=sizeof(test); i>0 ; i--){
		if(test[i-1]!=msg[i-1]){
			return 0;
		}
	}
	return 1;
}

/**
 * \brief checks if incoming message is a discovery message
 * \param msg pointer to the received message
 * \param dest Node the discovery was received from
 * \return 1 if msg is a discovery message or 0 if not
 */
uint8_t discovery_basic_is_discovery(uint8_t * msg, linkaddr_t * dest)
{
	uint8_t test[13]="DTN_DISCOVERY";
	uint8_t i;
	for (i=sizeof(test); i>0; i--) {
		if(test[i-1]!=msg[i-1]) {
			return 0;
		}
	}

	if( discovery_status == 0 ) {
		return 1;
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "DTN DISCOVERY");
	convergence_layer_send_discovery((uint8_t *) "DTN_HERE", 8, dest);

	return 1;
}

/**
 * \brief Enable discovery functionality
 */
void discovery_basic_enable()
{
	discovery_status = 1;
}

/**
 * \brief Disable discovery functionality
 * Prevents outgoing packets from being sent
 */
void discovery_basic_disable()
{
	discovery_status = 0;
}

/**
 * \brief DTN Network has received an incoming discovery packet
 * \param source Source address of the packet
 * \param payload Payload pointer of the packet
 * \param length Length of the payload
 */
void discovery_basic_receive(linkaddr_t * source, uint8_t * payload, uint8_t length)
{
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "received from %u:%u", source->u8[1], source->u8[0]);

	// Save all peer from which we receive packets to the active neighbours list
	discovery_basic_refresh_neighbour(source);

	// Either somebody wants to discover ourselves
	if( discovery_basic_is_discovery(payload, source) ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "is_discover");
		return;
	}

	// Or we have received a reply to our query
	if( discovery_basic_is_beacon(payload) ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "is_beacon");
		discovery_basic_neighbour_found(source);
	}
}

/**
 * \brief We have found a new neighbour, now go and notify the agent
 * \param neighbour Address of the newly found neighbour
 */
void discovery_basic_neighbour_found(linkaddr_t * neighbour)
{
	struct discovery_basic_neighbour_list_entry * entry;

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = entry->next) {
		if( entry->active &&
				linkaddr_cmp(&(entry->neighbour), neighbour) ) {
			break;
		}
	}

	if( entry == NULL ) {
		// Apparently previously unknown neighbour
	} else {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "sending DTN BEACON Event for %u.%u", neighbour->u8[0], neighbour->u8[1]);
//		process_post(&agent_process, dtn_beacon_event, &entry->neighbour);
		const event_container_t event = {
			.event = dtn_beacon_event,
			.linkaddr = &entry->neighbour
		};
		agent_send_event(&event);
	}

	// Once we have found a new neighbour, we will stop discovering other nodes
	discovery_basic_stop_pending();
}

/**
 * \brief Checks if ''neighbours'' is already known
 * Yes: refresh timestamp
 * No:  Create entry
 *
 * \param neighbour Address of the neighbour that should be refreshed
 */
void discovery_basic_refresh_neighbour(linkaddr_t * neighbour)
{
	struct discovery_basic_neighbour_list_entry * entry;

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = entry->next) {
		if( entry->active &&
				linkaddr_cmp(&(entry->neighbour), neighbour) ) {
			entry->timestamp = clock_time();
			return;
		}
	}

	discovery_basic_save_neighbour(neighbour);
}

/**
 * \brief Save neighbour to local cache
 * \param neighbour Address of the neighbour
 */
void discovery_basic_save_neighbour(linkaddr_t * neighbour)
{
	// If we know that neighbour already, no need to re-add it
	if( discovery_basic_is_neighbour(neighbour) ) {
		return;
	}

	struct discovery_basic_neighbour_list_entry * entry;
	entry = memb_alloc(&neighbour_mem);

	if( entry == NULL ) {
		// Now we have to delete an existing entry - which one to choose?
		struct discovery_basic_neighbour_list_entry * delete = list_head(neighbour_list);
		clock_time_t age = 0;

		// We select the entry with the hightest age
		for(entry = list_head(neighbour_list);
				entry != NULL;
				entry = entry->next) {
			if( entry->active && (clock_time() - entry->timestamp) > age ) {
				age = clock_time() - entry->timestamp;
				delete = entry;
			}
		}

		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Removing neighbour %u.%u to make room", delete->neighbour.u8[0], delete->neighbour.u8[1]);

		// And remove if from the list and free the memory
		memb_free(&neighbour_mem, delete);
		list_remove(neighbour_list, delete);

		// Now we can allocate memory again - has to work, no need to check return value
		entry = memb_alloc(&neighbour_mem);
	}

	// Clean the entry struct, so that "active" becomes zero
	memset(entry, 0, sizeof(struct discovery_basic_neighbour_list_entry));

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "DISCOVERY: Saving neighbour %u.%u", neighbour->u8[0], neighbour->u8[1]);
	entry->active = 1;
	linkaddr_copy(&(entry->neighbour), neighbour);
	entry->timestamp = clock_time();

	list_add(neighbour_list, entry);
}


/**
 * \brief Start to discover a neighbour
 *
 * \param dest Address of the neighbour
 * \return 1 if neighbour is already known in (likely) in range, 0 otherwise
 */
uint8_t discovery_basic_discover(linkaddr_t * dest)
{

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "agent asks to discover %u.%u", dest->u8[0], dest->u8[1]);

	// Check, if we already know this neighbour
	if(discovery_basic_is_neighbour(dest)) {
		return 1;
	}

	// Memorize, that we still have a discovery pending
	discovery_pending_start = 1;
//	process_poll(&discovery_process);
	vTaskResume(discovery_task);

	// Otherwise, send out a discovery beacon
	discovery_basic_send_discovery(dest);

	return 0;
}

/**
 * \brief Returns the list of currently known neighbours
 * \return Pointer to list with neighbours
 */
struct discovery_neighbour_list_entry * discovery_basic_list_neighbours()
{
	return list_head(neighbour_list);
}

/**
 * \brief Stops pending discoveries
 */
void discovery_basic_stop_pending()
{
	discovery_pending = 0;
//	etimer_stop(&discovery_pending_timer);
	xTimerStop(discovery_pending_timer, 0);
}

/**
 * \brief Starts a periodic rediscovery
 */
void discovery_basic_start_pending()
{
	discovery_pending = 1;
//	etimer_set(&discovery_pending_timer, DISCOVERY_CYCLE * CLOCK_SECOND);
	xTimerStart(discovery_pending_timer, 0);
}

void discovery_basic_start(clock_time_t duration, uint8_t index)
{
}

void discovery_basic_stop()
{
}

void discovery_basic_clear()
{
}


static void discovery_timeout(const TimerHandle_t timer)
{
	struct discovery_basic_neighbour_list_entry * entry;

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = entry->next) {
		if( entry->active && (xTaskGetTickCount() - entry->timestamp) > pdMS_TO_TICKS(DISCOVERY_NEIGHBOUR_TIMEOUT * 1000) ) {
			LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Neighbour %u.%u timed out: %lu vs. %lu = %lu", entry->neighbour.u8[0], entry->neighbour.u8[1], clock_time(), entry->timestamp, clock_time() - entry->timestamp);

			// Tell the CL that this neighbour has disappeared
			convergence_layer_neighbour_down(&entry->neighbour);

			memb_free(&neighbour_mem, entry);
			list_remove(neighbour_list, entry);
		}
	}

//	etimer_restart(&discovery_timeout_timer);
	// no longer needed because of auto reloading of the timer is active
}


static void discovery_pending_timeout(const TimerHandle_t timer)
{
	/**
	 * If we have a discovery pending, resend the beacon multiple times
	 */
	if( discovery_pending) {
		discovery_basic_send_discovery(NULL);
		discovery_pending ++;

		if( discovery_pending > (DISCOVERY_TRIES + 1)) {
			discovery_basic_stop_pending();
		} else {
//			etimer_restart(&discovery_pending_timer);
			xTimerReset(timer, 0);
		}
	}
}


/**
 * \brief Basic Discovery Persistent Process
 */
static void discovery_process(void* p)
{
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Basic Discovery process running");

	while(1) {
//		PROCESS_WAIT_EVENT();
		vTaskSuspend(NULL);

		/**
		 * In case we should start the periodic discovery, do it here and now
		 */
		if( discovery_pending_start ) {
			discovery_basic_start_pending();
			discovery_pending_start = 0;
		}
	}
}

const struct discovery_driver discovery_basic = {
		.name = "B_DISCOVERY",
		.init = discovery_basic_init,
		.is_neighbour = discovery_basic_is_neighbour,
		.enable = discovery_basic_enable,
		.disable = discovery_basic_disable,
		.receive = discovery_basic_receive,
		.alive = discovery_basic_refresh_neighbour,
		.dead = NULL,
		.discover = discovery_basic_discover,
		.neighbours = discovery_basic_list_neighbours,
		.stop_pending = discovery_basic_stop_pending,
		.start        = discovery_basic_start,
		.stop         = discovery_basic_stop,
		.clear        = discovery_basic_clear,
};
/** @} */
/** @} */
