 /**
 * \defgroup cl Convergence Layer
 *
 * @{
 */

/**
* \file IEEE 802.15.4 Convergence Layer Implementation
* \author Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
*/

#ifndef CONVERGENCE_LAYER_H
#define CONVERGENCE_LAYER_H

#include "rimeaddr.h"
#include "process.h"

/**
 * How many outgoing bundles can we queue?
 */
#define CONVERGENCE_LAYER_QUEUE 			10

/**
 * How many queue slots remain free for internal use?
 */
#define CONVERGENCE_LAYER_QUEUE_FREE 		(0.2 * CONVERGENCE_LAYER_QUEUE)

/**
 * How often shall we retransmit bundles before we notify routing
 */
#define CONVERGENCE_LAYER_RETRIES			3

/**
 * How long shall we wait for an app-layer ACK or NACK? [in seconds]
 */
#define CONVERGENCE_LAYER_TIMEOUT			5

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

/**
 * CL COMPAT VALUES
 */
#define CONVERGENCE_LAYER_COMPAT			0x00

/**
 * CL Header Types
 */
#define CONVERGENCE_LAYER_TYPE_DATA 		0x10
#define CONVERGENCE_LAYER_TYPE_DISCOVERY	0x20
#define CONVERGENCE_LAYER_TYPE_ACK 			0x30
#define CONVERGENCE_LAYER_TYPE_NACK 		0x00

/**
 * CL Packet Flags
 */
#define CONVERGENCE_LAYER_FLAGS_FIRST		0x02
#define CONVERGENCE_LAYER_FLAGS_LAST		0x01

/**
 * CL Field Masks
 */
#define CONVERGENCE_LAYER_MASK_COMPAT		0xC0
#define CONVERGENCE_LAYER_MASK_TYPE			0x30
#define CONVERGENCE_LAYER_MASK_SEQNO		0x0C
#define CONVERGENCE_LAYER_MASK_FLAGS		0x03

/**
 * CL Callback Status
 */
#define CONVERGENCE_LAYER_STATUS_OK			0x01
#define CONVERGENCE_LAYER_STATUS_NOACK		0x02
#define CONVERGENCE_LAYER_STATUS_NOSEND		0x04

/**
 * CL Priority Values
 */
#define CONVERGENCE_LAYER_PRIORITY_NORMAL	0x01
#define CONVERGENCE_LAYER_PRIORITY_HIGH		0x02

#define CONVERGENCE_LAYER_VALID_FLAG		0x7a03ab12UL

/**
 * Convergence Layer Process
 */
PROCESS_NAME(convergence_layer_process);

/**
 * Bundle Queue Entry
 */
struct transmit_ticket_t {
	struct transmit_ticket_t * next;

	uint8_t flags;
	uint8_t tries;
	rimeaddr_t neighbour;
	uint32_t bundle_number;
	uint8_t sequence_number;

	struct mmem * bundle;
};

int convergence_layer_init(void);

struct transmit_ticket_t * convergence_layer_get_transmit_ticket(void);
int convergence_layer_free_transmit_ticket(struct transmit_ticket_t * ticket);

int convergence_layer_enqueue_bundle(struct transmit_ticket_t * ticket);
int convergence_layer_send_discovery(uint8_t * payload, uint8_t length, rimeaddr_t * neighbour);

int convergence_layer_incoming_frame(rimeaddr_t * source, uint8_t * payload, uint8_t length);
int convergence_layer_status(void * pointer, uint8_t status);

int convergence_layer_delete_bundle(uint32_t bundle_number);

#endif /* CONVERGENCE_LAYER */

/** @} */
/** @} */
