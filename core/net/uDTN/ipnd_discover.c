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
#include "dtn_config.h"
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
#include "sdnv.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void ipnd_dis_refresh_neighbour(rimeaddr_t * neighbour);
void ipnd_dis_save_neighbour(rimeaddr_t * neighbour);

#define DISCOVERY_NEIGHBOUR_CACHE	3
#define DISCOVERY_CYCLE				1
#define DISCOVERY_NEIGHBOUR_TIMEOUT	(5*DISCOVERY_CYCLE)

#define IPND_FLAGS_SOURCE_EID		(1<<0)
#define IPND_FLAGS_SERVICE_BLOCK	(1<<1)
#define IPND_FLAGS_BLOOMFILTER		(1<<2)

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
static struct etimer discovery_timeout_timer;
static struct etimer discovery_cycle_timer;
uint16_t discovery_sequencenumber = 0;

void ipnd_dis_init()
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
uint8_t ipnd_dis_neighbour(rimeaddr_t * dest)
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

/**
 * Enable discovery functionality
 */
void ipnd_dis_enable()
{
	discovery_status = 1;
}

/**
 * Disable discovery functionality
 * Prevents outgoing packets from being sent
 */
void ipnd_dis_disable()
{
	discovery_status = 0;
}

uint8_t ipnd_parse_eid(uint32_t * eid, uint8_t * buffer, uint8_t length) {
	uint32_t sdnv_length = 0;
	int offset = 0;

	// int sdnv_decode(const uint8_t* bp, size_t len, uint32_t* val)
	offset += sdnv_decode(&buffer[offset], length - offset, &sdnv_length);

	// FIXME: sdnv_decode's return value is -1 - WTF?
	offset = 1;

	if( strncmp((char *) &buffer[offset], "ipn:", 4) == 0 ) {
		*eid = atoi((char *) &buffer[offset + 4]);
	} else {
		PRINTF("IPND: unknown EID format %s\n", &buffer[offset + 4]);
	}

	return offset + sdnv_length;
}

uint8_t ipnd_parse_service_block(uint8_t * buffer, uint8_t length) {
	return 0;
}

uint8_t ipnd_parse_bloomfilter(uint8_t * buffer, uint8_t length) {
	return 0;
}

/**
 * DTN Network has received an incoming packet, save the sender as neighbour
 */
void ipnd_dis_receive(rimeaddr_t * source, uint8_t * payload, uint8_t length)
{
	int offset = 0;

	// Save all peer from which we receive packets to the active neighbours list
	ipnd_dis_refresh_neighbour(source);

	// FIXME: Parse IP-ND format here

	// Version
	if( payload[offset++] != 0x02 ) {
		PRINTF("IPND: version mismatch\n");
		return;
	}

	// Flags
	uint8_t flags = payload[offset++];

	// Sequence Number
	uint16_t sequenceNumber = ((payload[offset] & 0xFF)<< 8) + (payload[offset+1] & 0xFF);
	offset += 2;

	uint32_t eid = 0;
	if( flags & IPND_FLAGS_SOURCE_EID ) {
		offset += ipnd_parse_eid(&eid, &payload[offset], length - offset);
	}

	if( flags & IPND_FLAGS_SERVICE_BLOCK ) {
		offset += ipnd_parse_service_block(&payload[offset], length - offset);
	}

	if( flags & IPND_FLAGS_BLOOMFILTER ) {
		offset += ipnd_parse_bloomfilter(&payload[offset], length - offset);
	}

	PRINTF("IPND: Discovery from %lu with flags %02X and seqNo %u\n", eid, flags, sequenceNumber);
}


void ipnd_dis_send() {
	uint8_t buffer[20];
	int offset = 0;

	// Clear the buffer
	memset(buffer, 0, 20);

	// Version
	buffer[offset++] = 0x02;

	// Flags
	buffer[offset++] = IPND_FLAGS_SOURCE_EID;

	// Beacon Sequence Number
	buffer[offset++] = (discovery_sequencenumber & 0xFF00) << 8;
	buffer[offset++] = (discovery_sequencenumber & 0x00FF) << 0;
	discovery_sequencenumber ++;

	// EID Length
	char EID[20];
	int len;
	len = sprintf(EID, "ipn:%lu", dtn_node_id);
	offset += sdnv_encode(len, &buffer[offset], 20 - offset);

	// EID
	memcpy(&buffer[offset], EID, len);
	offset += len;

	// Now: Send it
	rimeaddr_t destination = {{0, 0}}; // Broadcast
	dtn_send_discover(buffer, offset, &destination);
}

/**
 * Checks if ''neighbours'' is already known
 * Yes: refresh timestamp
 * No:  Create entry
 */
void ipnd_dis_refresh_neighbour(rimeaddr_t * neighbour)
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

	ipnd_dis_save_neighbour(neighbour);
}

/**
 * Save neighbour to local cache
 */
void ipnd_dis_save_neighbour(rimeaddr_t * neighbour)
{
	// If we know that neighbour already, no need to re-add it
	if( ipnd_dis_neighbour(neighbour) ) {
		return;
	}

	struct discovery_basic_neighbour_list_entry * entry;
	entry = memb_alloc(&neighbour_mem);

	if( entry == NULL ) {
		PRINTF("DISCOVERY: no more space for neighbours");
		return;
	}

	// Clean the entry struct, so that "active" becomes zero
	memset(entry, 0, sizeof(struct discovery_basic_neighbour_list_entry));

	PRINTF("DISCOVERY: Saving neighbour %u:%u \n", neighbour->u8[0], neighbour->u8[1]);
	entry->active = 1;
	rimeaddr_copy(&(entry->neighbour), neighbour);
	entry->timestamp = clock_time();

	list_add(neighbour_list, entry);

	// We have found a new neighbour, now go and notify the agent
	process_post(&agent_process, dtn_beacon_event, neighbour);
}

/**
 * Returns the list of currently known neighbours
 */
struct discovery_neighbour_list_entry * ipnd_dis_list_neighbours()
{
	return list_head(neighbour_list);
}

/**
 * Stops pending discoveries
 */
void ipnd_dis_stop_pending()
{

}

/**
 * IPND Discovery Persistent Process
 */
PROCESS_THREAD(discovery_process, ev, data)
{
	PROCESS_BEGIN();

	etimer_set(&discovery_timeout_timer, DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND);
	etimer_set(&discovery_cycle_timer, DISCOVERY_CYCLE * CLOCK_SECOND);
	PRINTF("DISCOVERY: process running\n");

	while(1) {
		PROCESS_WAIT_EVENT();

		if( etimer_expired(&discovery_timeout_timer) ) {
			struct discovery_basic_neighbour_list_entry * entry;

			for(entry = list_head(neighbour_list);
					entry != NULL;
					entry = entry->next) {
				if( entry->active && (clock_time() - entry->timestamp) > (DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND) ) {
					PRINTF("DISCOVERY: Neighbour %u:%u timed out: %lu vs. %lu = %lu\n", entry->neighbour.u8[0], entry->neighbour.u8[1], clock_time(), entry->timestamp, clock_time() - entry->timestamp);

					memb_free(&neighbour_mem, entry);
					list_remove(neighbour_list, entry);
				}
			}

			etimer_reset(&discovery_timeout_timer);
		}

		/**
		 * Regularly send out our discovery beacon
		 */
		if( etimer_expired(&discovery_cycle_timer) ) {
			ipnd_dis_send();
			etimer_reset(&discovery_cycle_timer);
		}
	}

	PROCESS_END();
}

const struct discovery_driver ipnd_discovery = {
	"IPND_DISCOVERY",
	ipnd_dis_init,
	ipnd_dis_neighbour,
	ipnd_dis_enable,
	ipnd_dis_disable,
	ipnd_dis_receive,
	ipnd_dis_refresh_neighbour,
	ipnd_dis_neighbour,
	ipnd_dis_list_neighbours,
	ipnd_dis_stop_pending,
};
/** @} */
/** @} */

