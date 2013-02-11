/**
 * \addtogroup discovery 
 * @{
 */

/**
 * \defgroup discovery_ipnd IP-ND discovery module
 *
 * @{
 */

/**
 * \file
 * \brief IP-ND compliant discovery module
 * IP-ND = http://tools.ietf.org/html/draft-irtf-dtnrg-ipnd-01
 *
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <string.h> // for memset

#include "net/netstack.h"
#include "net/packetbuf.h" 
#include "net/rime/rimeaddr.h"
#include "clock.h"
#include "net/mac/frame802154.h" // for IEEE802154_PANID
#include "logging.h"
#include "random.h"

#include "dtn_network.h"
#include "agent.h"
#include "sdnv.h"
#include "statistics.h"
#include "convergence_layer.h"
#include "eid.h"

#include "discovery.h"

void discovery_ipnd_refresh_neighbour(rimeaddr_t * neighbour);
void discovery_ipnd_save_neighbour(rimeaddr_t * neighbour);

#define DISCOVERY_NEIGHBOUR_CACHE	3
#define DISCOVERY_CYCLE			5	
#define DISCOVERY_NEIGHBOUR_TIMEOUT	(5*DISCOVERY_CYCLE)
#define DISCOVERY_IPND_SERVICE		"lowpancl"
#define DISCOVERY_IPND_BUFFER_LEN 	60
#define DISCOVERY_IPND_WHITELIST	0


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
	unsigned long timestamp_last;
	unsigned long timestamp_discovered;
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

rimeaddr_t discovery_whitelist[DISCOVERY_IPND_WHITELIST];

/**
 * \brief IPND Discovery init function (called by agent)
 */
void discovery_ipnd_init()
{
	// Enable discovery module
	discovery_status = 1;

	// Initialize the neighbour list
	list_init(neighbour_list);

	// Initialize the neighbour memory block
	memb_init(&neighbour_mem);

#if DISCOVERY_IPND_WHITELIST > 0
	// Clear the discovery whitelist
	memset(&discovery_whitelist, 0, sizeof(rimeaddr_t) * DISCOVERY_IPND_WHITELIST);

 // Fill the datastructure here

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "Whitelist enabled");
#endif

	// Start discovery process
	process_start(&discovery_process, NULL);
}

/** 
 * \brief Checks if address is currently listed a neighbour
 * \param dest Address of potential neighbour
 * \return 1 if neighbour, 0 otherwise
 */
uint8_t discovery_ipnd_is_neighbour(rimeaddr_t * dest)
{
	struct discovery_basic_neighbour_list_entry * entry;

	if( discovery_status == 0 ) {
		// Not initialized yet
		return 0;
	}

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
 * \brief Enable discovery functionality
 */
void discovery_ipnd_enable()
{
	discovery_status = 1;
}

/**
 * \brief Disable discovery functionality
 * Prevents outgoing packets from being sent
 */
void discovery_ipnd_disable()
{
	discovery_status = 0;
}

/**
 * \brief Parses an incoming EID in an IPND frame
 * \param eid Pointer where the EID will be stored
 * \param buffer Pointer to the incoming EID
 * \param length Length of the Pointer
 * \return Length of the parsed EID
 */
uint8_t discovery_ipnd_parse_eid(uint32_t * eid, uint8_t * buffer, uint8_t length) {
	int ret = 0;

	/* Parse EID */
	ret = eid_parse_host_length(buffer, length, eid);
	if( ret < 0 ) {
		return 0;
	}

	return ret;
}

/**
 * \brief Dummy function that could parse the IPND service block
 * \param buffer Pointer to the block
 * \param length Length of the block
 * \return dummy
 */
uint8_t discovery_ipnd_parse_service_block(uint8_t * buffer, uint8_t length) {
	return 0;
}

/**
 * \brief Dummy function that could parse the IPND bloom filter
 * \param buffer Pointer to the filter
 * \param length Length of the filter
 * \return dummy
 */
uint8_t discovery_ipnd_parse_bloomfilter(uint8_t * buffer, uint8_t length) {
	return 0;
}

/**
 * \brief DTN Network has received an incoming discovery packet
 * \param source Source address of the packet
 * \param payload Payload pointer of the packet
 * \param length Length of the payload
 */
void discovery_ipnd_receive(rimeaddr_t * source, uint8_t * payload, uint8_t length)
{
	int offset = 0;

	if( discovery_status == 0 ) {
		// Not initialized yet
		return;
	}

	if( length < 3 ) {
		// IPND must have at least 3 bytes
		return;
	}

	// Save all peer from which we receive packets to the active neighbours list
	discovery_ipnd_refresh_neighbour(source);

	// Version
	if( payload[offset++] != 0x02 ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "IPND version mismatch");
		return;
	}

	// Flags
	uint8_t flags = payload[offset++];

	// Sequence Number
	uint16_t sequenceNumber = ((payload[offset] & 0xFF) << 8) + (payload[offset+1] & 0xFF);
	offset += 2;

	uint32_t eid = 0;
	if( flags & IPND_FLAGS_SOURCE_EID ) {
		offset += discovery_ipnd_parse_eid(&eid, &payload[offset], length - offset);
	}

	if( flags & IPND_FLAGS_SERVICE_BLOCK ) {
		offset += discovery_ipnd_parse_service_block(&payload[offset], length - offset);
	}

	if( flags & IPND_FLAGS_BLOOMFILTER ) {
		offset += discovery_ipnd_parse_bloomfilter(&payload[offset], length - offset);
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Discovery from %lu with flags %02X and seqNo %u", eid, flags, sequenceNumber);
}

/**
 * \brief Send out IPND beacon
 */
void discovery_ipnd_send() {
	uint8_t ipnd_buffer[DISCOVERY_IPND_BUFFER_LEN];
	char string_buffer[20];
	int offset = 0;
	int len;

	// Clear the ipnd_buffer
	memset(ipnd_buffer, 0, DISCOVERY_IPND_BUFFER_LEN);

	// Version
	ipnd_buffer[offset++] = 0x02;

	// Flags
	ipnd_buffer[offset++] = IPND_FLAGS_SOURCE_EID | IPND_FLAGS_SERVICE_BLOCK;

	// Beacon Sequence Number
	ipnd_buffer[offset++] = (discovery_sequencenumber & 0xFF00) << 8;
	ipnd_buffer[offset++] = (discovery_sequencenumber & 0x00FF) << 0;
	discovery_sequencenumber ++;

	/**
	 * Add node's EID
	 */
	offset += eid_create_host_length(dtn_node_id, &ipnd_buffer[offset], DISCOVERY_IPND_BUFFER_LEN - offset);

	/**
	 * Add static Service block
	 */
	// We have one static service
	ipnd_buffer[offset++] = 1; // Number of services

	// The service is called DISCOVERY_IPND_SERVICE
	len = sprintf(string_buffer, DISCOVERY_IPND_SERVICE);
	offset += sdnv_encode(len, &ipnd_buffer[offset], DISCOVERY_IPND_BUFFER_LEN - offset);
	memcpy(&ipnd_buffer[offset], string_buffer, len);
	offset += len;

	// We exploit ip and port here
	len = sprintf(string_buffer, "ip=%lu;port=%u;", dtn_node_id, IEEE802154_PANID);
	offset += sdnv_encode(len, &ipnd_buffer[offset], DISCOVERY_IPND_BUFFER_LEN - offset);
	memcpy(&ipnd_buffer[offset], string_buffer, len);
	offset += len;

	// Now: Send it
	rimeaddr_t destination = {{0, 0}}; // Broadcast
	convergence_layer_send_discovery(ipnd_buffer, offset, &destination);
}

/**
 * \brief Checks if ''neighbours'' is already known
 * Yes: refresh timestamp
 * No:  Create entry
 *
 * \param neighbour Address of the neighbour that should be refreshed
 */
void discovery_ipnd_refresh_neighbour(rimeaddr_t * neighbour)
{
#if DISCOVERY_IPND_WHITELIST > 0
	int i;
	int found = 0;
	for(i=0; i<DISCOVERY_IPND_WHITELIST; i++) {
		if( rimeaddr_cmp(&discovery_whitelist[i], neighbour) ) {
			found = 1;
			break;
		}
	}

	if( !found ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "Ignoring peer %u.%u, not on whitelist", neighbour->u8[0], neighbour->u8[1]);
		return;
	}
#endif

	struct discovery_basic_neighbour_list_entry * entry;

	if( discovery_status == 0 ) {
		// Not initialized yet
		return;
	}

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = entry->next) {
		if( entry->active &&
				rimeaddr_cmp(&(entry->neighbour), neighbour) ) {
			entry->timestamp_last = clock_seconds();
			return;
		}
	}

	discovery_ipnd_save_neighbour(neighbour);
}

/**
 * \brief Marks a neighbour as 'dead' after multiple transmission attempts have failed
 * \param neighbour Address of the neighbour
 */
void discovery_ipnd_delete_neighbour(rimeaddr_t * neighbour)
{
	struct discovery_basic_neighbour_list_entry * entry;

	if( discovery_status == 0 ) {
		// Not initialized yet
		return;
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Neighbour %u.%u disappeared", neighbour->u8[0], neighbour->u8[1]);

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = entry->next) {
		if( entry->active &&
				rimeaddr_cmp(&(entry->neighbour), neighbour) ) {

			// Notify the statistics module
			statistics_contacts_down(&entry->neighbour, entry->timestamp_last - entry->timestamp_discovered);

			memb_free(&neighbour_mem, entry);
			list_remove(neighbour_list, entry);

			break;
		}
	}
}

/**
 * \brief Save neighbour to local cache
 * \param neighbour Address of the neighbour
 */
void discovery_ipnd_save_neighbour(rimeaddr_t * neighbour)
{
	if( discovery_status == 0 ) {
		// Not initialized yet
		return;
	}

	// If we know that neighbour already, no need to re-add it
	if( discovery_ipnd_is_neighbour(neighbour) ) {
		return;
	}

	struct discovery_basic_neighbour_list_entry * entry;
	entry = memb_alloc(&neighbour_mem);

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "no more space for neighbours");
		return;
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Found new neighbour %u.%u", neighbour->u8[0], neighbour->u8[1]);

	// Clean the entry struct, so that "active" becomes zero
	memset(entry, 0, sizeof(struct discovery_basic_neighbour_list_entry));

	entry->active = 1;
	rimeaddr_copy(&(entry->neighbour), neighbour);
	entry->timestamp_last = clock_seconds();
	entry->timestamp_discovered = clock_seconds();

	// Notify the statistics module
	statistics_contacts_up(neighbour);

	list_add(neighbour_list, entry);

	// We have found a new neighbour, now go and notify the agent
	process_post(&agent_process, dtn_beacon_event, neighbour);
}

/**
 * \brief Returns the list of currently known neighbours
 * \return Pointer to list with neighbours
 */
struct discovery_neighbour_list_entry * discovery_ipnd_list_neighbours()
{
	return list_head(neighbour_list);
}

/**
 * \brief Stops pending discoveries
 */
void discovery_ipnd_stop_pending()
{

}

/**
 * \brief IPND Discovery Persistent Process
 */
PROCESS_THREAD(discovery_process, ev, data)
{
	PROCESS_BEGIN();

	etimer_set(&discovery_timeout_timer, DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND);
	etimer_set(&discovery_cycle_timer, DISCOVERY_CYCLE * CLOCK_SECOND);
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Discovery process running");

	while(1) {
		PROCESS_WAIT_EVENT();

		if( etimer_expired(&discovery_timeout_timer) ) {
			struct discovery_basic_neighbour_list_entry * entry;

			for(entry = list_head(neighbour_list);
					entry != NULL;
					entry = entry->next) {
				if( entry->active && (clock_seconds() - entry->timestamp_last) > (DISCOVERY_NEIGHBOUR_TIMEOUT + 1) ) {
					LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Neighbour %u.%u timed out: %lu vs. %lu = %lu", entry->neighbour.u8[0], entry->neighbour.u8[1], clock_time(), entry->timestamp_last, clock_time() - entry->timestamp_last);
					discovery_ipnd_delete_neighbour(&entry->neighbour);
				}
			}

			etimer_reset(&discovery_timeout_timer);
		}

		/**
		 * Regularly send out our discovery beacon
		 */
		if( etimer_expired(&discovery_cycle_timer) ) {
			discovery_ipnd_send();
	
			/**
			 * Slight variance in the discovery interval to ensure, that even two nodes in a 
			 * simulator that have been started simentanously will find each other one day
			 */
			unsigned short random = random_rand();

			if( random < 0.15 * RANDOM_RAND_MAX ) {
				etimer_set(&discovery_cycle_timer, (DISCOVERY_CYCLE - 1) * CLOCK_SECOND);
			} else if( random > 0.7 * RANDOM_RAND_MAX ) {
				etimer_set(&discovery_cycle_timer, (DISCOVERY_CYCLE + 1) * CLOCK_SECOND);
			} else {
				etimer_restart(&discovery_cycle_timer);
			}
		}
	}

	PROCESS_END();
}

const struct discovery_driver discovery_ipnd = {
	"IPND_DISCOVERY",
	discovery_ipnd_init,
	discovery_ipnd_is_neighbour,
	discovery_ipnd_enable,
	discovery_ipnd_disable,
	discovery_ipnd_receive,
	discovery_ipnd_refresh_neighbour,
	discovery_ipnd_delete_neighbour,
	discovery_ipnd_is_neighbour,
	discovery_ipnd_list_neighbours,
	discovery_ipnd_stop_pending,
};
/** @} */
/** @} */

