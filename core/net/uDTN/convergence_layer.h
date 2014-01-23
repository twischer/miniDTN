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

#include "contiki.h"
#include "rimeaddr.h"
#include "process.h"
#include "mmem.h"

/**
 * How many outgoing bundles can we queue?
 */
#define CONVERGENCE_LAYER_QUEUE 				10

/**
 * How many queue slots remain free for internal use?
 */
#define CONVERGENCE_LAYER_QUEUE_FREE 			(0.2 * CONVERGENCE_LAYER_QUEUE)

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
 * How long shell we wait before retransmitting an app-layer ACK or NACK? [in seconds]
 */
#define CONVERGENCE_LAYER_RETRANSMIT_TIMEOUT	0.5

/**
 * How often shall we retransmit?
 */
#define CONVERGENCE_LAYER_RETRANSMIT_TRIES		(CONVERGENCE_LAYER_TIMEOUT / CONVERGENCE_LAYER_RETRANSMIT_TIMEOUT)

/**
 * Specify the maximum bundle size that we guarantee to support
 * Memory is reserved to serialize a bundle of this size
 */
#ifdef CONVERGENCE_LAYER_CONF_MAX_SIZE
#define CONVERGENCE_LAYER_MAX_SIZE CONVERGENCE_LAYER_CONF_MAX_SIZE
#else
#define CONVERGENCE_LAYER_MAX_SIZE 300
#endif

/**
 * Shall we support segmentation?
 */
#ifdef CONVERGENCE_LAYER_CONF_SEGMENTATION
#define CONVERGENCE_LAYER_SEGMENTATION CONVERGENCE_LAYER_CONF_SEGMENTATION
#else
#define CONVERGENCE_LAYER_SEGMENTATION 1
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
#define CONVERGENCE_LAYER_TYPE_TEMP_NACK	0x40

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
#define CONVERGENCE_LAYER_STATUS_FATAL		0x08

/**
 * CL Priority Values
 */
#define CONVERGENCE_LAYER_PRIORITY_NORMAL	0x01
#define CONVERGENCE_LAYER_PRIORITY_HIGH		0x02

/**
 * Maximum payload length of one outgoing frame
 *
 * IEEE 802.15.4 MAC Frames: 127 Byte
 * Frame Control Field    2 Byte
 * Sequence Number        1 Byte
 * Dst PAN                2 Byte
 * Dst Address            2 Byte
 * Src PAN                2 Byte
 * Src Address            2 Byte
 * Security Header        0 Byte
 * Frame Check Sequence   2 Byte
 * --- TOTAL:            13 Byte
 *
 * With PAN Compression: 11 Byte
 *
 * 127 Byte - 11 Byte = 116 Byte
 */
#define CONVERGENCE_LAYER_MAX_LENGTH 116

/**
 * Convergence Layer Process
 */
PROCESS_NAME(convergence_layer_process);

/**
 * Bundle Queue Entry
 */
struct transmit_ticket_t {
	struct transmit_ticket_t * next;

	uint16_t flags;
	uint8_t tries;
	uint8_t failed_tries;
	rimeaddr_t neighbour;
	uint32_t bundle_number;
	uint8_t sequence_number;
	clock_time_t timestamp;

#if CONVERGENCE_LAYER_SEGMENTATION
	int offset_sent;
	int offset_acked;
	struct mmem buffer;
#endif

	struct mmem * bundle;
};

int convergence_layer_init(void);

struct transmit_ticket_t * convergence_layer_get_transmit_ticket(void);
int convergence_layer_free_transmit_ticket(struct transmit_ticket_t * ticket);

int convergence_layer_enqueue_bundle(struct transmit_ticket_t * ticket);
int convergence_layer_send_discovery(uint8_t * payload, uint8_t length, rimeaddr_t * neighbour);

int convergence_layer_incoming_frame(rimeaddr_t * source, uint8_t * payload, uint8_t length, packetbuf_attr_t rssi);
int convergence_layer_status(void * pointer, uint8_t status);

int convergence_layer_delete_bundle(uint32_t bundle_number);

int convergence_layer_neighbour_down(rimeaddr_t * neighbour);

#endif /* CONVERGENCE_LAYER */

/** @} */
/** @} */
