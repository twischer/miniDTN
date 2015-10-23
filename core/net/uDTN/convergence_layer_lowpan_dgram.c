#include "convergence_layer_lowpan_dgram.h"

#include "lib/logging.h"
#include "dtn_network.h"
#include "discovery.h"
#include "agent.h"
#include "convergence_layer_dgram.h"


/**
 * CL Field Masks
 */
#define CONVERGENCE_LAYER_MASK_COMPAT		0xC0
#define CONVERGENCE_LAYER_MASK_TYPE			0x30
#define CONVERGENCE_LAYER_MASK_SEQNO		0x0C
#define CONVERGENCE_LAYER_MASK_FLAGS		0x03

/**
 * CL COMPAT VALUES
 */
#define CONVERGENCE_LAYER_COMPAT			0x00


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


struct lowpan_dgram_hdr
{
	uint8_t compat : 2;
	uint8_t type : 2;
	uint8_t seqno : 2;
	uint8_t flags : 2;
}  __attribute__ ((packed));


/**
 * MUTEX to avoid flooding the MAC layer with outgoing bundles
 */
static bool convergence_layer_transmitting = false;


static int convergence_layer_lowpan_dgram_init()
{
	return convergence_layer_dgram_init();
}


static size_t convergence_layer_lowpan_dgram_max_payload_length(void)
{
	/* the gram lowpan clayer needs 1 byte */
	return CONVERGENCE_LAYER_MAX_LENGTH - sizeof(struct lowpan_dgram_hdr);
}


static uint8_t convergence_layer_lowpan_dgram_next_sequence_number(const uint8_t last_seqno)
{
	return (last_seqno + 1) % 4;
}


static int convergence_layer_lowpan_dgram_send_discovery(const uint8_t* const payload, const size_t length)
{
	uint8_t * buffer = NULL;

	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Sending broadcast discovery");

	/* If we are currently transmitting or waiting for an ACK, do nothing */
	if(convergence_layer_transmitting) {
		return -1;
	}

	/* Get our buffer */
	buffer = dtn_network_get_buffer();
	if( buffer == NULL ) {
		return -1;
	}

	/* Discovery Prefix */
	buffer[0] = CONVERGENCE_LAYER_TYPE_DISCOVERY;

	/* Copy the discovery message and set the length */
	// TODO check for max dtn buffer length
	memcpy(buffer + sizeof(struct lowpan_dgram_hdr), payload, length);

	/* Now we are transmitting */
	convergence_layer_transmitting = true;

	/* Send it out via the MAC */
	static linkaddr_t bcast_addr = {{0, 0}};
	dtn_network_send(&bcast_addr, length + sizeof(struct lowpan_dgram_hdr), NULL);

	return 1;
}

/**
 * @brief convergence_layer_lowpan_dgram_send_bundle
 * @param dest
 * @param sequence_number
 * @param flags
 * @param payload
 * @param length
 * @param reference
 * @return <0 an error occured
 *          0 bundle was not send, because of locked driver
 *          1 bundle was send
 */
static int convergence_layer_lowpan_dgram_send_bundle(const cl_addr_t* const dest, const int sequence_number, const uint8_t flags,
											const uint8_t* const payload, const size_t length, const void* const reference)
{
	configASSERT(dest->clayer == &clayer_lowpan_dgram);

	/* only send the bundle, if no other bundle is sending */
	if (convergence_layer_transmitting) {
		return 0;
	}

	/* Now we are transmitting */
	convergence_layer_transmitting = true;

	/* Get the outgoing network buffer */
	uint8_t* const buffer = dtn_network_get_buffer();
	if( buffer == NULL ) {
		return -1;
	}

	/* add one byte for the dgram:lowpan header */
	const uint8_t length_to_send = length + sizeof(struct lowpan_dgram_hdr);

	/* fail if the buffer is not big enough */
	if (length_to_send > dtn_network_get_buffer_length()) {
		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(dest, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_WRN, "DTN network buffer to small for bundle (seq %u, flags %x, len %lu) to  addr %s",
			sequence_number, flags, length_to_send, addr_str);
		return -2;
	}

	/* Initialize the header field */
	buffer[0] = CONVERGENCE_LAYER_TYPE_DATA & CONVERGENCE_LAYER_MASK_TYPE;

	/* Put the sequence number for this bundle into the outgoing header */
	buffer[0] |= (sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
	buffer[0] |= flags & CONVERGENCE_LAYER_MASK_FLAGS;

	/* copy the payload */
	memcpy(&buffer[1], payload, length);

	/* And send it out */
	// TODO remove const cast
	dtn_network_send((linkaddr_t*)&dest->lowpan, length_to_send, (void*)reference);

	return 1;
}

/**
 * @brief convergence_layer_lowpan_dgram_send_ack
 * @param destination
 * @param sequence_number
 * @param type
 * @param reference
 * @return <0 an error occured
 *          0 bundle was not send, because of locked driver
 *          1 bundle was send
 */
static int convergence_layer_lowpan_dgram_send_ack(const cl_addr_t* const destination, const int sequence_number, const int type,
											 const void* const reference)
{
	configASSERT(destination->clayer == &clayer_lowpan_dgram);

	/* only send the bundle, if no other bundle is sending */
	if (convergence_layer_transmitting) {
		return 0;
	}

	/* Now we are transmitting */
	convergence_layer_transmitting = true;

	/* Get our buffer */
	uint8_t* const buffer = dtn_network_get_buffer();
	if( buffer == NULL ) {
		return -1;
	}

	if( type == CONVERGENCE_LAYER_TYPE_ACK ) {
		// Construct the ACK
		buffer[0] = 0;
		buffer[0] |= CONVERGENCE_LAYER_TYPE_ACK & CONVERGENCE_LAYER_MASK_TYPE;
		buffer[0] |= (sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
	} else if( type == CONVERGENCE_LAYER_TYPE_NACK ) {
		// Construct the NACK
		buffer[0] = 0;
		buffer[0] |= CONVERGENCE_LAYER_TYPE_NACK & CONVERGENCE_LAYER_MASK_TYPE;
		buffer[0] |= (sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
	} else if( type == CONVERGENCE_LAYER_TYPE_TEMP_NACK ) {
		// Construct the temporary NACK
		buffer[0] = 0;
		buffer[0] |= CONVERGENCE_LAYER_TYPE_NACK & CONVERGENCE_LAYER_MASK_TYPE;
		buffer[0] |= (sequence_number << 2) & CONVERGENCE_LAYER_MASK_SEQNO;
		buffer[0] |= (CONVERGENCE_LAYER_FLAGS_FIRST) & CONVERGENCE_LAYER_MASK_FLAGS; // This flag indicates a temporary nack
	}

	/* Send it out via the MAC */
	dtn_network_send((linkaddr_t*)&destination->lowpan, sizeof(struct lowpan_dgram_hdr), (void*)reference);

	return 1;
}


int convergence_layer_lowpan_dgram_status(const void* const pointer, const uint8_t outcome)
{
	/* Something has been sent, so the radio is not blocked anymore */
	convergence_layer_transmitting = false;

	return convergence_layer_dgram_status(pointer, outcome);
}


static int convergence_layer_lowpan_dgram_incoming_frame(const cl_addr_t* const source, const uint8_t* const payload, const size_t length, const packetbuf_attr_t rssi)
{
	configASSERT(source->clayer == &clayer_lowpan_dgram);

	uint8_t data_length = 0;
	uint8_t header;

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(source, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming frame from %s (header 0x%02x)", addr_str, payload[0]);

	/* Notify the discovery module, that we have seen a peer */
	DISCOVERY.alive(source);
	// TODO call alive_eid, if discovery entry does not already exist

	/* Check the COMPAT information */
	if( (payload[0] & CONVERGENCE_LAYER_MASK_COMPAT) != CONVERGENCE_LAYER_COMPAT ) {
		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_INF, "Ignoring incoming frame from %s", addr_str);
		return -1;
	}

	header = payload[0];
	const uint8_t* const data_pointer = payload + sizeof(struct lowpan_dgram_hdr);
	data_length = length - sizeof(struct lowpan_dgram_hdr);

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_DATA ) {
		/* is data */
		const int flags = (header & CONVERGENCE_LAYER_MASK_FLAGS) >> 0;
		const int sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		return convergence_layer_dgram_incoming_data(source, data_pointer, data_length, rssi, sequence_number, flags);
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_DISCOVERY ) {
		/* is discovery */
		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming discovery frame from %s", addr_str);

		DISCOVERY.receive(source, data_pointer, data_length);

		return 1;
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_ACK ) {
		/* is ACK */
		int flags = 0;
		int sequence_number = 0;

		flags = (header & CONVERGENCE_LAYER_MASK_FLAGS) >> 0;
		sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming Ack frame from %s with SeqNo %u", addr_str, sequence_number);

		convergence_layer_dgram_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_ACK, flags);

		return 1;
	}

	if( (header & CONVERGENCE_LAYER_MASK_TYPE) == CONVERGENCE_LAYER_TYPE_NACK ) {
		/* is NACK */
		int flags = 0;
		int sequence_number = 0;

		flags = (header & CONVERGENCE_LAYER_MASK_FLAGS) >> 0;
		sequence_number = (header & CONVERGENCE_LAYER_MASK_SEQNO) >> 2;

		char addr_str[CL_ADDR_STRING_LENGTH];
		cl_addr_string(source, addr_str, sizeof(addr_str));
		LOG(LOGD_DTN, LOG_CL, LOGL_DBG, "Incoming Nack frame from %s with SeqNo %u", addr_str, sequence_number);

		convergence_layer_dgram_parse_ackframe(source, data_pointer, data_length, sequence_number, CONVERGENCE_LAYER_TYPE_NACK, flags);

		return 1;
	}

	return 0;
}


const struct convergence_layer clayer_lowpan_dgram = {
	.name = "dgram:lowpan",
	.init = convergence_layer_lowpan_dgram_init,
	.max_payload_length = convergence_layer_lowpan_dgram_max_payload_length,
	.next_seqno = convergence_layer_lowpan_dgram_next_sequence_number,
	.send_discovery = convergence_layer_lowpan_dgram_send_discovery,
	.send_ack = convergence_layer_lowpan_dgram_send_ack,
	.send_bundle = convergence_layer_lowpan_dgram_send_bundle,
	.input = convergence_layer_lowpan_dgram_incoming_frame
};
