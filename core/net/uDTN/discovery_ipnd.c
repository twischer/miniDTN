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
#include "sys/ctimer.h"

#include "dtn_apps.h"
#include "dtn_network.h"
#include "agent.h"
#include "sdnv.h"
#include "statistics.h"
#include "convergence_layer.h"
#include "eid.h"

#include "discovery.h"

void discovery_ipnd_refresh_neighbour(rimeaddr_t * neighbour);
void discovery_ipnd_save_neighbour(rimeaddr_t * neighbour);
void discovery_ipnd_remove_stale_neighbours(void * ptr);
void discovery_ipnd_print_list();

#define DISCOVERY_NEIGHBOUR_CACHE	3
#define DISCOVERY_NEIGHBOUR_TIMEOUT	25
#define DISCOVERY_IPND_SERVICE		"lowpancl"
#define DISCOVERY_IPND_BUFFER_LEN 	70
#define DISCOVERY_IPND_WHITELIST	0


#define IPND_FLAGS_SOURCE_EID		(1<<0)
#define IPND_FLAGS_SERVICE_BLOCK	(1<<1)
#define IPND_FLAGS_BLOOMFILTER		(1<<2)

/**
 * This "internal" struct extends the discovery_neighbour_list_entry struct with
 * more attributes for internal use
 */
struct discovery_ipnd_neighbour_list_entry {
	struct discovery_ipnd_neighbour_list_entry *next;
	rimeaddr_t neighbour;
	unsigned long timestamp_last;
	unsigned long timestamp_discovered;
};

/**
 * List and memory blocks to save information about neighbours
 */
LIST(neighbour_list);
MEMB(neighbour_mem, struct discovery_ipnd_neighbour_list_entry, DISCOVERY_NEIGHBOUR_CACHE);

uint8_t discovery_status = 0;
static struct ctimer discovery_timeout_timer;
uint16_t discovery_sequencenumber = 0;

rimeaddr_t discovery_whitelist[DISCOVERY_IPND_WHITELIST];

/**
 * \brief IPND Discovery init function (called by agent)
 */
void discovery_ipnd_init()
{
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

	// Set the neighbour timeout timer
	ctimer_set(&discovery_timeout_timer, DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND, discovery_ipnd_remove_stale_neighbours, NULL);

	// Enable discovery module
	discovery_status = 1;
}

/** 
 * \brief Checks if address is currently listed a neighbour
 * \param dest Address of potential neighbour
 * \return 1 if neighbour, 0 otherwise
 */
uint8_t discovery_ipnd_is_neighbour(rimeaddr_t * dest)
{
	struct discovery_ipnd_neighbour_list_entry * entry;

	if( discovery_status == 0 ) {
		// Not initialized yet
		return 0;
	}

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		if( rimeaddr_cmp(&entry->neighbour, dest) ) {
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
}

/**
 * \brief Disable discovery functionality
 * Prevents outgoing packets from being sent
 */
void discovery_ipnd_disable()
{
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
 * \brief Function that could parse the IPND service block
 * \param buffer Pointer to the block
 * \param length Length of the block
 * \return the number of decoded bytes
 */
uint8_t discovery_ipnd_parse_service_block(uint32_t eid, uint8_t * buffer, uint8_t length) {
	uint32_t offset, num_services, i, sdnv_len, tag_len, data_len;
	uint8_t *tag_buf;
	int h;

	// decode the number of services
	sdnv_len = sdnv_decode(buffer, length, &num_services);
	offset = sdnv_len;
	buffer += sdnv_len;

	// iterate through all services
	for (i = 0; num_services > i; ++i) {
		// decode service tag length
		sdnv_len = sdnv_decode(buffer, length - offset, &tag_len);
		offset += sdnv_len;
		buffer += sdnv_len;

		// decode service tag string
		tag_buf = buffer;
		offset += tag_len;
		buffer += tag_len;

		// decode service content length
		sdnv_len = sdnv_decode(buffer, length - offset, &data_len);
		offset += sdnv_len;
		buffer += sdnv_len;

		// Allow all registered DTN APPs to parse the IPND service block
		for(h=0; dtn_apps[h] != NULL; h++) {
			if( dtn_apps[h]->parse_ipnd_service_block == NULL ) {
				continue;
			}

			dtn_apps[h]->parse_ipnd_service_block(eid, tag_buf, tag_len, buffer, data_len);
		}

		offset += data_len;
		buffer += data_len;
	}

	return offset;
}

/**
 * \brief Dummy function that could parse the IPND bloom filter
 * \param eid The endpoint which broadcasted this bloomfilter
 * \param buffer Pointer to the filter
 * \param length Length of the filter
 * \return dummy
 */
uint8_t discovery_ipnd_parse_bloomfilter(uint32_t eid, uint8_t * buffer, uint8_t length) {
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

	/* Prevent compiler warning about unused variable */
	(void) sequenceNumber;

	uint32_t eid = 0;
	if( flags & IPND_FLAGS_SOURCE_EID ) {
		offset += discovery_ipnd_parse_eid(&eid, &payload[offset], length - offset);
	}

	if( flags & IPND_FLAGS_SERVICE_BLOCK ) {
		offset += discovery_ipnd_parse_service_block(eid, &payload[offset], length - offset);
	}

	if( flags & IPND_FLAGS_BLOOMFILTER ) {
		offset += discovery_ipnd_parse_bloomfilter(eid, &payload[offset], length - offset);
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Discovery from %u.%u (ipn:%lu) with flags %02X and seqNo %u", source->u8[0], source->u8[1], eid, flags, sequenceNumber);
}

/**
 * \brief Send out IPND beacon
 */
void discovery_ipnd_send() {
	uint8_t ipnd_buffer[DISCOVERY_IPND_BUFFER_LEN];
	int offset = 0;
	int h = 0;
	uint8_t * services = NULL;

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
	services = &ipnd_buffer[offset++]; // This is a pointer onto the location of the service counter in the outgoing buffer
	*services = 0; // Start with 0 services

	// Allow all registered DTN APPs to add an IPND service block
	for(h=0; dtn_apps[h] != NULL; h++) {
		if( dtn_apps[h]->add_ipnd_service_block == NULL ) {
			continue;
		}

		*services += dtn_apps[h]->add_ipnd_service_block(&ipnd_buffer[offset], DISCOVERY_IPND_BUFFER_LEN, &offset);
	}

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

	struct discovery_ipnd_neighbour_list_entry * entry;

	if( discovery_status == 0 ) {
		// Not initialized yet
		return;
	}

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		if( rimeaddr_cmp(&entry->neighbour, neighbour) ) {
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
	struct discovery_ipnd_neighbour_list_entry * entry;

	if( discovery_status == 0 ) {
		// Not initialized yet
		return;
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Neighbour %u.%u disappeared", neighbour->u8[0], neighbour->u8[1]);

	// Tell the CL that this neighbour has disappeared
	convergence_layer_neighbour_down(neighbour);

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		if( rimeaddr_cmp(&entry->neighbour, neighbour) ) {

			// Notify the statistics module
			statistics_contacts_down(&entry->neighbour, entry->timestamp_last - entry->timestamp_discovered);

			list_remove(neighbour_list, entry);
			memb_free(&neighbour_mem, entry);

			return;
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

	struct discovery_ipnd_neighbour_list_entry * entry;
	entry = memb_alloc(&neighbour_mem);

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "no more space for neighbours");
		return;
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Found new neighbour %u.%u", neighbour->u8[0], neighbour->u8[1]);

	// Clean the entry struct
	memset(entry, 0, sizeof(struct discovery_ipnd_neighbour_list_entry));

	rimeaddr_copy(&entry->neighbour, neighbour);
	entry->timestamp_last = clock_seconds();
	entry->timestamp_discovered = clock_seconds();

	// Notify the statistics module
	statistics_contacts_up(neighbour);

	list_add(neighbour_list, entry);

	// We have found a new neighbour, now go and notify the agent
	process_post(&agent_process, dtn_beacon_event, &entry->neighbour);
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

void discovery_ipnd_start(clock_time_t duration, uint8_t index)
{
	// Send at the beginning of a cycle
	discovery_ipnd_send();
}

void discovery_ipnd_stop()
{
	// Send at the end of a cycle
	discovery_ipnd_send();
}

void discovery_ipnd_clear()
{
	struct discovery_ipnd_neighbour_list_entry * entry;

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Clearing neighbour list");

	while(list_head(neighbour_list) != NULL) {
		entry = list_head(neighbour_list);

		// Notify the statistics module
		statistics_contacts_down(&entry->neighbour, entry->timestamp_last - entry->timestamp_discovered);

		convergence_layer_neighbour_down(&entry->neighbour);
		list_remove(neighbour_list, entry);
		memb_free(&neighbour_mem, entry);
	}
}

void discovery_ipnd_remove_stale_neighbours(void * ptr)
{
	struct discovery_ipnd_neighbour_list_entry * entry;
	int changed = 1;

	while( changed ) {
		changed = 0;

		for(entry = list_head(neighbour_list);
				entry != NULL;
				entry = list_item_next(entry)) {
			if( (clock_seconds() - entry->timestamp_last) > DISCOVERY_NEIGHBOUR_TIMEOUT ) {
				LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Neighbour %u.%u timed out: %lu vs. %lu = %lu", entry->neighbour.u8[0], entry->neighbour.u8[1], clock_time(), entry->timestamp_last, clock_time() - entry->timestamp_last);
				discovery_ipnd_delete_neighbour(&entry->neighbour);
				changed = 1;
				break;
			}
		}
	}

	ctimer_set(&discovery_timeout_timer, DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND, discovery_ipnd_remove_stale_neighbours, NULL);
}

void discovery_ipnd_print_list()
{
	struct discovery_ipnd_neighbour_list_entry * entry;

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		printf("%p: %u.%u -> %p\n", entry, entry->neighbour.u8[0], entry->neighbour.u8[1], entry->next);
	}
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
		discovery_ipnd_start,
		discovery_ipnd_stop,
		discovery_ipnd_clear
};
/** @} */
/** @} */

