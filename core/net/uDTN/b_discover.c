/**
 * \addtogroup discovery 
 * @{
 */

 /**
 * \defgroup ipnd_biscover  IP-ND discovery module
 *
 * @{
 */

/**
* \file
* IP-ND compliant discovery module
* IP-ND = http://tools.ietf.org/html/draft-irtf-dtnrg-ipnd-01
*
* \author Wolf-Bastian Pšttner (poettner@ibr.cs.tu-bs.de)
*/

#include "custody-signal.h"
#include "dtn-network.h"
#include "clock.h"
#include "net/netstack.h"
#include "net/packetbuf.h" 
#include "net/rime/rimeaddr.h"

#include "agent.h"
#include "status-report.h"
#include "dtn-network.h" 
#include "discovery.h"
#include <string.h> // for memset

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void b_dis_neighbour_found(rimeaddr_t * neighbour);
void b_dis_refresh_neighbour(rimeaddr_t * neighbour);
void b_dis_save_neighbour(rimeaddr_t * neighbour);
void b_dis_stop_pending();

#define DISCOVERY_NEIGHBOUR_CACHE	3
#define DISCOVERY_NEIGHBOUR_TIMEOUT	5
#define DISCOVERY_CYCLE 			0.2
#define DISCOVERY_TRIES				5

/**
 * This "internal" struct extends the discovery_neighbour_list_entry struct with
 * more attributes for internal use
 */
struct discovery_basic_neighbour_list_entry {
	struct discovery_basic_neighbour_list_entry *next;
	rimeaddr_t neighbour;
	clock_time_t timestamp;
	uint8_t active;
};

PROCESS(discovery_process, "DISCOVERY process");

/**
 * List and memory blocks to save information about neighbours
 */
LIST(neighbour_list);
MEMB(neighbour_mem, struct discovery_basic_neighbour_list_entry, DISCOVERY_NEIGHBOUR_CACHE);

uint8_t discovery_status = 0;
uint8_t discovery_pending = 0;
uint8_t discovery_pending_start = 0;
static struct etimer discovery_timeout_timer;
static struct etimer discovery_pending_timer;

void b_dis_init()
{
	// Enable discovery module
	discovery_status = 1;

	// Initialize the neighbour list
	list_init(neighbour_list);

	// Initialize the neighbour memory block
	memb_init(&neighbour_mem);

	// Start discovery process
	process_start(&discovery_process, NULL);
}

/** 
* \brief sends a discovery message 
* \param bundle pointer to a bundle (not used here)
*/
uint8_t b_dis_neighbour(rimeaddr_t * dest)
{
	struct discovery_basic_neighbour_list_entry * entry;

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = entry->next) {
		if( entry->active &&
				rimeaddr_cmp(&(entry->neighbour), dest) ) {
			return 1;
		}
	}

	return 0;
}

void b_dis_send(rimeaddr_t * destination)
{
	if( discovery_status == 0 ) {
		return;
	}

	rimeaddr_t dest={{0,0}};
	PRINTF("dtn_send_discover\n");

	convergence_layer_send_discovery((uint8_t *) "DTN_DISCOVERY", 13, &dest);
}

/**
* \brief checks if msg is an answer to a discovery message
* \param msg pointer to the received message
* \return 1 if msg is an answer or 0 if not
*/
uint8_t b_dis_is_beacon(uint8_t * msg)
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
* \brief hecks if msg is a discovery message
* \param msg pointer to the received message
* \return 1 if msg is a discovery message or 0 if not
*/
uint8_t b_dis_is_discover(uint8_t * msg, rimeaddr_t * dest)
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

	PRINTF("DTN DISCOVERY\n");
	convergence_layer_send_discovery((uint8_t *) "DTN_HERE", 8, dest);

	return 1;
}

/**
 * Enable discovery functionality
 */
void b_dis_enable()
{
	discovery_status = 1;
}

/**
 * Disable discovery functionality
 * Prevents outgoing packets from being sent
 */
void b_dis_disable()
{
	discovery_status = 0;
}

/**
 * DTN Network has received an incoming packet, save the sender as neighbour
 */
void b_dis_receive(rimeaddr_t * source, uint8_t * payload, uint8_t length)
{
	PRINTF("DISCOVERY: received from %u:%u\n", source->u8[1], source->u8[0]);

	// Save all peer from which we receive packets to the active neighbours list
	b_dis_refresh_neighbour(source);

	// Either somebody wants to discover ourselves
	if( b_dis_is_discover(payload, source) ) {
		PRINTF("is_discover\n");
		return;
	}

	// Or we have received a reply to our query
	if( b_dis_is_beacon(payload) ) {
		PRINTF("is_beacon\n");
		b_dis_neighbour_found(source);
	}
}

/**
 * We have found a new neighbour, now go and notify the agent
 */
void b_dis_neighbour_found(rimeaddr_t * neighbour)
{
	PRINTF("DISCOVERY: sending DTN BEACON Event for %u.%u\n", neighbour->u8[0], neighbour->u8[1]);
	process_post(&agent_process, dtn_beacon_event, neighbour);

	// Once we have found a new neighbour, we will stop discovering other nodes
	b_dis_stop_pending();
}

/**
 * Checks if ''neighbours'' is already known
 * Yes: refresh timestamp
 * No:  Create entry
 */
void b_dis_refresh_neighbour(rimeaddr_t * neighbour)
{
	struct discovery_basic_neighbour_list_entry * entry;

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = entry->next) {
		if( entry->active &&
				rimeaddr_cmp(&(entry->neighbour), neighbour) ) {
			entry->timestamp = clock_time();
			return;
		}
	}

	b_dis_save_neighbour(neighbour);
}

/**
 * Save neighbour to local cache
 */
void b_dis_save_neighbour(rimeaddr_t * neighbour)
{
	// If we know that neighbour already, no need to re-add it
	if( b_dis_neighbour(neighbour) ) {
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

		PRINTF("DISCOVERY: Removing neighbour %u.%u to make room\n", delete->neighbour.u8[0], delete->neighbour.u8[1]);

		// And remove if from the list and free the memory
		memb_free(&neighbour_mem, delete);
		list_remove(neighbour_list, delete);

		// Now we can allocate memory again - has to work, no need to check return value
		entry = memb_alloc(&neighbour_mem);
	}

	// Clean the entry struct, so that "active" becomes zero
	memset(entry, 0, sizeof(struct discovery_basic_neighbour_list_entry));

	PRINTF("DISCOVERY: Saving neighbour %u:%u \n", neighbour->u8[0], neighbour->u8[1]);
	entry->active = 1;
	rimeaddr_copy(&(entry->neighbour), neighbour);
	entry->timestamp = clock_time();

	list_add(neighbour_list, entry);
}


/**
 * Start to discover a neighbour
 * Returns 1 if neighbour is already known in (likely) in range
 * Returns 0 otherwise
 */
uint8_t b_dis_discover(rimeaddr_t * dest)
{
	PRINTF("DISCOVERY: agent asks to discover %u:%u\n", dest->u8[1], dest->u8[0]);

	// Check, if we already know this neighbour
	if(b_dis_neighbour(dest)) {
		return 1;
	}

	// Memorize, that we still have a discovery pending
	discovery_pending_start = 1;
	process_poll(&discovery_process);

	// Otherwise, send out a discovery beacon
	b_dis_send(dest);

	return 0;
}

/**
 * Returns the list of currently known neighbours
 */
struct discovery_neighbour_list_entry * b_dis_list_neighbours()
{
	return list_head(neighbour_list);
}

/**
 * Stops pending discoveries
 */
void b_dis_stop_pending()
{
	discovery_pending = 0;
	etimer_stop(&discovery_pending_timer);
}

/**
 * Starts a periodic rediscovery
 */
void b_dis_start_pending()
{
	discovery_pending = 1;
	etimer_set(&discovery_pending_timer, DISCOVERY_CYCLE * CLOCK_SECOND);
}

/**
 * Basic Discovery Persistent Process
 */
PROCESS_THREAD(discovery_process, ev, data)
{
	PROCESS_BEGIN();

	etimer_set(&discovery_timeout_timer, DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND);
	PRINTF("DISCOVERY: process running\n");

	while(1) {
		PROCESS_WAIT_EVENT();

		if( etimer_expired(&discovery_timeout_timer) ) {
			struct discovery_basic_neighbour_list_entry * entry;

			for(entry = list_head(neighbour_list);
					entry != NULL;
					entry = entry->next) {
				if( entry->active && (clock_time() - entry->timestamp) > (DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND) ) {
					PRINTF("DISCOVERY: Neighbour %u:%u timed out: %lu vs. %lu = %lu\n", entry->neighbour.u8[1], entry->neighbour.u8[0], clock_time(), entry->timestamp, clock_time() - entry->timestamp);

					memb_free(&neighbour_mem, entry);
					list_remove(neighbour_list, entry);
				}
			}

			etimer_reset(&discovery_timeout_timer);
		}

		/**
		 * If we have a discovery pending, resend the beacon multiple times
		 */
		if( discovery_pending && etimer_expired(&discovery_pending_timer) ) {
			b_dis_send(NULL);
			discovery_pending ++;

			if( discovery_pending > (DISCOVERY_TRIES + 1)) {
				b_dis_stop_pending();
			} else {
				etimer_reset(&discovery_pending_timer);
			}
		}

		/**
		 * In case we should start the periodic discovery, do it here and now
		 */
		if( discovery_pending_start ) {
			b_dis_start_pending();
			discovery_pending_start = 0;
		}
	}

	PROCESS_END();
}

const struct discovery_driver b_discovery = {
	.name = "B_DISCOVERY",
	.init = b_dis_init,
	.is_neighbour = b_dis_neighbour,
	.enable = b_dis_enable,
	.disable = b_dis_disable,
	.receive = b_dis_receive,
	.alive = b_dis_refresh_neighbour,
	.dead = NULL,
	.discover = b_dis_discover,
	.neighbours = b_dis_list_neighbours,
	.stop_pending = b_dis_stop_pending,
};
/** @} */
/** @} */
