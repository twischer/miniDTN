 /**
 * \defgroup statistics Statistics module
 *
 * @{
 */

/**
 * \file uDTN Statistics Module
 * \author Wolf-Bastian Pšttner (poettner@ibr.cs.tu-bs.de)
 */


#ifndef STATISTICS_H
#define STATISTICS_H

#include "net/rime/rimeaddr.h"

#define STATISTICS_ELEMENTS	4

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

uint16_t statistics_setup(uint16_t interval);
uint8_t statistics_get_bundle(uint8_t * buffer, uint8_t maximum_length);
void statistics_reset(void);

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
