 /**
 * \defgroup statistics Statistics module
 *
 * @{
 */

/**
 * \file
 * \brief uDTN Statistics Module header
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include "contiki.h"
#include "net/rime/rimeaddr.h"

#ifdef STATISTICS_CONF_ELEMENTS
#define STATISTICS_ELEMENTS STATISTICS_CONF_ELEMENTS
#else
#define STATISTICS_ELEMENTS	0
#endif

#ifdef STATISTICS_CONF_PERIOD
#define STATISTICS_PERIOD STATISTICS_CONF_PERIOD
#else
#define STATISTICS_PERIOD 0
#endif

// 8 bytes per contact
#ifdef STATISTICS_CONF_CONTACTS
#define STATISTICS_CONTACTS STATISTICS_CONF_CONTACTS
#else
#define STATISTICS_CONTACTS 0
#endif

extern process_event_t dtn_statistics_overrun;

struct statistics_element_t
{
	uint8_t	bundles_incoming;
	uint8_t bundles_outgoing;
	uint8_t bundles_generated;
	uint8_t bundles_delivered;

	uint8_t storage_bundles;
	uint8_t contacts_count;

	uint16_t contacts_duration;
	uint16_t storage_memory;
};

struct contact_element_t
{
	uint8_t peer;
	uint16_t time_difference;
	uint16_t duration;
};

uint16_t statistics_setup();
uint8_t statistics_get_bundle(uint8_t * buffer, uint8_t maximum_length);
uint8_t statistics_get_contacts_bundle(uint8_t * buffer, uint8_t maximum_length);
void statistics_reset(void);
void statistics_reset_contacts();

void statistics_bundle_incoming(uint8_t count);
void statistics_bundle_outgoing(uint8_t count);
void statistics_bundle_generated(uint8_t count);
void statistics_bundle_delivered(uint8_t count);

void statistics_storage_bundles(uint8_t bundles);
void statistics_storage_memory(uint16_t free);

void statistics_contacts_up(rimeaddr_t * peer);
void statistics_contacts_down(rimeaddr_t * peer, uint16_t duration);

#endif /* STATISTICS_H */

/** @} */
