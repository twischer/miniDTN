/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup cl IEEE 802.15.4 Convergence Layer
 *
 * @{
 */

/**
 * \file
 * \brief IEEE 802.15.4 Convergence Layer Implementation
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef CONVERGENCE_LAYER_H
#define CONVERGENCE_LAYER_H

#include <stdbool.h>
#include "FreeRTOS.h"

#include "net/linkaddr.h"
#include "net/packetbuf.h"
#include "lib/mmem.h"

#include "cl_address.h"

/**
 * How many outgoing bundles can we queue?
 */
#define CONVERGENCE_LAYER_QUEUE 				10

/**
 * How many queue slots remain free for internal use?
 * 1/5 is the same as 20%, but is not using the float unit
 */
#define CONVERGENCE_LAYER_QUEUE_FREE 			(CONVERGENCE_LAYER_QUEUE / 5)

/**
 * How often shall we retransmit bundles before we notify routing
 */
#define CONVERGENCE_LAYER_RETRIES				4

/**
 * How often shell we retry to transmit, if it has not been transmitted at all?
 */
#define CONVERGENCE_LAYER_FAILED_RETRIES		15

/**
 * How long shall we wait for an app-layer ACK or NACK? [in seconds]
 */
#ifndef CONVERGENCE_LAYER_TIMEOUT
#define CONVERGENCE_LAYER_TIMEOUT				5
#endif

/**
 * How long shall we wait for the completion of a multipart bundle when receiving? [in seconds]
 */
#define CONVERGENCE_LAYER_MULTIPART_TIMEOUT		10

/**
 * How long shell we wait before retransmitting an app-layer ACK or NACK? [in milli seconds]
 */
#define CONVERGENCE_LAYER_RETRANSMIT_TIMEOUT	500

/**
 * How often shall we retransmit?
 */
#define CONVERGENCE_LAYER_RETRANSMIT_TRIES		(CONVERGENCE_LAYER_TIMEOUT * 1000 / CONVERGENCE_LAYER_RETRANSMIT_TIMEOUT)

/**
 * Specify the maximum bundle size that we guarantee to support
 * Memory is reserved to serialize a bundle of this size
 */
#ifdef CONVERGENCE_LAYER_CONF_MAX_SIZE
#define CONVERGENCE_LAYER_MAX_SIZE CONVERGENCE_LAYER_CONF_MAX_SIZE
#else
#define CONVERGENCE_LAYER_MAX_SIZE ETH_MAX_ETH_PAYLOAD
#endif

/**
 * Bundle queue flags
 */
#define CONVERGENCE_LAYER_QUEUE_ACTIVE 		0x01
#define CONVERGENCE_LAYER_QUEUE_IN_TRANSIT 	0x02
#define CONVERGENCE_LAYER_QUEUE_ACK_PEND	0x04
#define CONVERGENCE_LAYER_QUEUE_DONE		0x08
#define CONVERGENCE_LAYER_QUEUE_FAIL		0x10
#define CONVERGENCE_LAYER_QUEUE_ACK			0x20
#define CONVERGENCE_LAYER_QUEUE_NACK		0x40
#define CONVERGENCE_LAYER_QUEUE_TEMP_NACK	0x80
#define CONVERGENCE_LAYER_QUEUE_MULTIPART	0x100
#define CONVERGENCE_LAYER_QUEUE_MULTIPART_RECV	0x200

/**
 * CL Header Types
 */
#define CONVERGENCE_LAYER_TYPE_DATA 		0x10
#define CONVERGENCE_LAYER_TYPE_DISCOVERY	0x20
#define CONVERGENCE_LAYER_TYPE_ACK 			0x30
#define CONVERGENCE_LAYER_TYPE_NACK 		0x00
#define CONVERGENCE_LAYER_TYPE_TEMP_NACK	0x40

/**
 * CL Packet Flags
 */
#define CONVERGENCE_LAYER_FLAGS_FIRST		0x02
#define CONVERGENCE_LAYER_FLAGS_LAST		0x01

/**
 * CL Callback Status
 */
#define CONVERGENCE_LAYER_STATUS_OK			0x01
#define CONVERGENCE_LAYER_STATUS_NOACK		0x02
#define CONVERGENCE_LAYER_STATUS_NOSEND		0x04
#define CONVERGENCE_LAYER_STATUS_FATAL		0x08

/**
 * CL Priority Values
 */
#define CONVERGENCE_LAYER_PRIORITY_NORMAL	0x01
#define CONVERGENCE_LAYER_PRIORITY_HIGH		0x02


/**
 * Bundle Queue Entry
 */
struct transmit_ticket_t {
	struct transmit_ticket_t * next;

	uint16_t flags;
	uint8_t tries;
	uint8_t failed_tries;
	cl_addr_t neighbour;
	uint32_t bundle_number;
	uint8_t sequence_number;
	TickType_t timestamp;

	int offset_sent;
	int offset_acked;
	struct mmem buffer;

	struct mmem * bundle;
};


bool convergence_layer_init(void);

int convergence_layer_free_transmit_ticket(struct transmit_ticket_t * ticket);
struct transmit_ticket_t * convergence_layer_get_transmit_ticket();


int convergence_layer_enqueue_bundle(struct transmit_ticket_t * ticket);

int convergence_layer_incoming_data(const cl_addr_t* const source, const uint8_t* const data_pointer, const size_t data_length,
									const packetbuf_attr_t rssi, const int sequence_number, const int flags);
int convergence_layer_parse_ackframe(const cl_addr_t* const source, const uint8_t* const payload, const uint8_t length,
											const uint8_t sequence_number, const uint8_t type, const uint8_t flags);
int convergence_layer_status(const void* const pointer, const uint8_t outcome);

int convergence_layer_delete_bundle(uint32_t bundle_number);

int convergence_layer_neighbour_down(const cl_addr_t* const neighbour);

#endif /* CONVERGENCE_LAYER */

/** @} */
/** @} */
