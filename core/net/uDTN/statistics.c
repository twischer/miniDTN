 /**
 * \addtogroup statistics Statistics module
 *
 * @{
 */

/**
 * \file
 * \brief uDTN Statistics Module implementation
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <string.h> // for memcpy

#include "agent.h"
#include "contiki.h"
#include "bundle.h"
#include "logging.h"

#include "statistics.h"

process_event_t dtn_statistics_overrun;

unsigned long statistics_timestamp = 0;
struct statistics_element_t statistics_array[STATISTICS_ELEMENTS];
struct contact_element_t statistics_contacts[STATISTICS_CONTACTS];
struct process * statistics_event_process = NULL;
uint8_t contacts_pointer = 0;
unsigned long contacts_timestamp = 0;

/**
 * \brief Internal function to find out, into which array slot the information is written
 */
uint8_t statistics_get_pointer()
{
#if STATISTICS_PERIOD > 0
	// Calculate since when we are recording statistics
	unsigned long elapsed = clock_seconds() - statistics_timestamp;

	// Find out in which "slot" of the array we are currently writing
	uint8_t ptr = elapsed / STATISTICS_PERIOD;

	if( ptr >= STATISTICS_ELEMENTS ) {
		ptr = STATISTICS_ELEMENTS - 1;
	}

	return ptr;
#else
	return 0;
#endif
}

/**
 * \brief Init function to be called by application
 * \return Seconds after which the statistics have to be sent off
 */
uint16_t statistics_setup(struct process * process)
{
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "setup()");

	// Reset the results (just to be sure)
	statistics_reset();

	// Store to which process we have to send the events
	statistics_event_process = process;

	// Allocate our event
	dtn_statistics_overrun = process_alloc_event();

	return STATISTICS_PERIOD * STATISTICS_ELEMENTS;
}

/**
 * \brief Copy the statistics bundle into the provided buffer
 * \return Length of the payload
 */
uint8_t statistics_get_bundle(uint8_t * buffer, uint8_t maximum_length)
{
	int offset = 0;

	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "get_bundle(%p, %u)", buffer, maximum_length);

	// Store the timestamp of this period
	memcpy(buffer + offset, &statistics_timestamp, sizeof(statistics_timestamp));
	offset += sizeof(statistics_timestamp);

	// Store how long each step is
	buffer[offset++] = (STATISTICS_PERIOD & 0x00FF) >> 0;
	buffer[offset++] = (STATISTICS_PERIOD & 0xFF00) >> 8;

	// Store how many periods are recorded
	buffer[offset++] = STATISTICS_ELEMENTS;

	// And now copy over the struct
	memcpy(buffer + offset, &statistics_array, sizeof(struct statistics_element_t) * STATISTICS_ELEMENTS);
	offset += sizeof(struct statistics_element_t) * STATISTICS_ELEMENTS;

	return offset;
}

/**
 * \brief Copy the contacts bundle into the provided buffer
 * \return Length of the payload
 */
uint8_t statistics_get_contacts_bundle(uint8_t * buffer, uint8_t maximum_length)
{
	int offset = 0;

	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "get_contacts_bundle(%p, %u)", buffer, maximum_length);

	// This should never happen
	if( contacts_pointer > STATISTICS_CONTACTS ) {
		contacts_pointer = STATISTICS_CONTACTS;
	}

	// Store how many entries are following
	buffer[offset++] = contacts_pointer;

	buffer[offset++] = (contacts_timestamp & 0x000000FF) >>  0;
	buffer[offset++] = (contacts_timestamp & 0x0000FF00) >>  8;
	buffer[offset++] = (contacts_timestamp & 0x00FF0000) >> 16;
	buffer[offset++] = (contacts_timestamp & 0xFF000000) >> 24;

	// And now copy over the struct
	memcpy(buffer + offset, &statistics_contacts, sizeof(struct contact_element_t) * contacts_pointer);
	offset += sizeof(struct contact_element_t) * contacts_pointer;

	return offset;
}

/**
 * \brief Resets the contact information, to be called by application after contacts bundle has been retrieved
 */
void statistics_reset_contacts()
{
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "reset_contacts()");

	memset(statistics_contacts, 0, sizeof(struct contact_element_t) * STATISTICS_CONTACTS);
	contacts_pointer = 0;
	contacts_timestamp = 0;
}

/**
 * \brief Resets the statistics, to be called by application after bundle has been retrieved
 */
void statistics_reset(void)
{
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "reset()");

	// Nullify the whole array
	memset(statistics_array, 0, sizeof(struct statistics_element_t) * STATISTICS_ELEMENTS);

	// Record the current timestamp
	statistics_timestamp = clock_seconds();
}

/**
 * \brief new bundle has been received
 */
void statistics_bundle_incoming(uint8_t count)
{
#if STATISTICS_ELEMENTS > 0
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "bundle_incoming(%u)", count);

	statistics_array[statistics_get_pointer()].bundles_incoming += count;
#endif
}

/**
 * \brief Bundle has been sent out
 */
void statistics_bundle_outgoing(uint8_t count)
{
#if STATISTICS_ELEMENTS > 0
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "bundle_outgoing(%u)", count);

	statistics_array[statistics_get_pointer()].bundles_outgoing += count;
#endif
}

/**
 * \brief Bundle was generated locally
 */
void statistics_bundle_generated(uint8_t count)
{
#if STATISTICS_ELEMENTS > 0
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "bundle_generated(%u)", count);

	statistics_array[statistics_get_pointer()].bundles_generated += count;
#endif
}

/**
 * \brief Bundle was delivered locally
 */
void statistics_bundle_delivered(uint8_t count)
{
#if STATISTICS_ELEMENTS > 0
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "statistics_bundle_delivered(%u)", count);

	statistics_array[statistics_get_pointer()].bundles_delivered += count;
#endif
}

/**
 * \brief Provide the number of bundles currently in storage
 */
void statistics_storage_bundles(uint8_t bundles)
{
#if STATISTICS_ELEMENTS > 0
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "storage_bundles(%u)", bundles);

	statistics_array[statistics_get_pointer()].storage_bundles = bundles;
#endif
}

/**
 * \brief Provide the amount of memory that is free
 */
void statistics_storage_memory(uint16_t free)
{
#if STATISTICS_ELEMENTS > 0
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "storage_memory(%u)", free);

	statistics_array[statistics_get_pointer()].storage_memory = free;
#endif
}

/**
 * \brief A new neighbour has been found
 */
void statistics_contacts_up(rimeaddr_t * peer)
{
#if STATISTICS_ELEMENTS > 0
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "contacts_up(%u.%u)", peer->u8[0], peer->u8[1]);
#endif
}

/**
 * \brief Neighbour disappeared
 */
void statistics_contacts_down(rimeaddr_t * peer, uint16_t duration)
{
#if STATISTICS_ELEMENTS > 0
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "contacts_down(%u.%u, %u)", peer->u8[0], peer->u8[1], duration);

	statistics_array[statistics_get_pointer()].contacts_count ++;
	statistics_array[statistics_get_pointer()].contacts_duration += duration;
#endif

#if STATISTICS_CONTACTS > 0
	uint32_t id = convert_rime_to_eid(peer);

	// We record only contacts with nodes that have a lower id than ourselves to avoid recording contacts twice (on both sides)
	if( duration == 0 || id > dtn_node_id ) {
		return;
	}

	if( contacts_pointer >= STATISTICS_CONTACTS ) {
		// This can only happen, when nobody picks up the data (or if somebody forgot to reset)
		contacts_pointer = STATISTICS_CONTACTS - 1;
	}

	// Record the contact
	unsigned long timestamp = clock_seconds() - duration;

	if( contacts_timestamp == 0 && contacts_pointer == 0 ) {
		contacts_timestamp = timestamp;
	}

	statistics_contacts[contacts_pointer].time_difference = (uint16_t) (timestamp - contacts_timestamp);
	statistics_contacts[contacts_pointer].peer = (uint8_t) id;
	statistics_contacts[contacts_pointer].duration = duration;
	contacts_pointer ++;

	// Avoid overrunning the array
	if( contacts_pointer >= STATISTICS_CONTACTS && statistics_event_process != NULL ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "contacts full, sending event");
		process_post(statistics_event_process, dtn_statistics_overrun, NULL);
	} else if( statistics_event_process == NULL ) {
		// Nobody is interested in our data anyway, start from the beginning
		LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "contacts full, clearing array");
		statistics_reset_contacts();
	}
#endif
}
/** @} */
