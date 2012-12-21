/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup bundle Bundle Memory Representation
 *
 * @{
 */

/**
 * \file 
 * \brief this file defines the bundle memory representation
 *
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdint.h>

#include "contiki.h"
#include "mmem.h"
#include "net/rime/rimeaddr.h"
#include "process.h"

#ifndef __BUNDLE_H__
#define __BUNDLE_H__


//primary block defines
#define VERSION 0
#define FLAGS 1
#define LENGTH 2
#define DEST_NODE 3
#define DEST_SERV 4
#define SRC_NODE 5
#define SRC_SERV 6
#define REP_NODE 7
#define REP_SERV 8
#define CUST_NODE 9
#define CUST_SERV 10
#define TIME_STAMP 11
#define TIME_STAMP_SEQ_NR 12
#define LIFE_TIME 13
#define DIRECTORY_LEN 14
#define FRAG_OFFSET 15
#define APP_DATA_LEN 16

/* Bundle Block Types */
#define BUNDLE_BLOCK_TYPE_PAYLOAD		0x01

/* Bundle deletion reasons */
#define REASON_NO_INFORMATION			0x00
#define REASON_LIFETIME_EXPIRED			0x01
#define REASON_UNIDIRECTIONAL_LINK		0x02
#define REASON_TRANSMISSION_CANCELED	0x03
#define REASON_DEPLETED_STORAGE			0x04
#define REASON_DEST_EID_UNINTELLIGBLE	0x05
#define REASON_NO_ROUTE					0x06
#define REASON_NO_TIMELY_CONTACT		0x07
#define REASON_BLOCK_UNINTELLIGBLE		0x08
#define REASON_DELIVERED				0xFF

/* Bundle Flag defines */
#define BUNDLE_FLAG_FRAGMENT			0x0001
#define BUNDLE_FLAG_ADM_REC				0x0002
#define BUNDLE_FLAG_DONT_FRAG			0x0004
#define BUNDLE_FLAG_CUST_REQ			0x0008
#define BUNDLE_FLAG_SINGLETON			0x0010
#define BUNDLE_FLAG_ACK_REQ				0x0020
/* Bit 6 reserved */
#define BUNDLE_FLAG_PRIOL				0x0080
#define BUNDLE_FLAG_PRIOH				0x0100
/* Bit 9 - 13 reserved */
#define BUNDLE_FLAG_REP_RECV			0x4000
#define BUNDLE_FLAG_REP_CUST			0x8000
#define BUNDLE_FLAG_REP_FWD				0x10000
#define BUNDLE_FLAG_REP_DELIV			0x20000
#define BUNDLE_FLAG_REP_DELETE			0x40000
/* Bit 19 and 20 reserved */

#define BUNDLE_FLAG_REPORT				(BUNDLE_FLAG_REP_RECV | BUNDLE_FLAG_REP_CUST | BUNDLE_FLAG_REP_FWD | BUNDLE_FLAG_REP_DELIV | BUNDLE_FLAG_REP_DELETE)

/* Block Flag defines */
#define BUNDLE_BLOCK_FLAG_NULL			0x00
#define BUNDLE_BLOCK_FLAG_REPL			0x01
#define BUNDLE_BLOCK_FLAG_STAT			0x02
#define BUNDLE_BLOCK_FLAG_DEL			0x04
#define BUNDLE_BLOCK_FLAG_LAST			0x08
#define BUNDLE_BLOCK_FLAG_DISC			0x10
#define BUNDLE_BLOCK_FLAG_NOTPR			0x20
#define BUNDLE_BLOCK_FLAG_EID			0x40

//payload block defines
#define DATA 							17

// Enable Timing debug output
#define DEBUG_H 0

struct bundle_block_t {
	uint8_t type;
	uint32_t flags;

	/* FIXME: EID References are unsupported */

	/* Variable array at the end to hold the payload
	 * Size is uint8 despite being an SDNV because
	 * IEEE-805.15.4 limits the size here. */
	uint8_t block_size;
	uint8_t payload[];
} __attribute__((packed));

/**
* \brief this struct defines the bundle for internal processing
*/
struct bundle_t{
	uint8_t custody;
	uint8_t del_reason;

	uint32_t bundle_num;

	uint32_t rec_time;

	uint32_t flags;
	uint32_t dst_node;
	uint32_t dst_srv;
	uint32_t src_node;
	uint32_t src_srv;
	uint32_t rep_node;
	uint32_t rep_srv;
	uint32_t cust_node;
	uint32_t cust_srv;
	uint32_t tstamp;
	uint32_t tstamp_seq;

	uint32_t lifetime;

	uint32_t frag_offs;
	uint32_t app_len;

	struct process * source_process;
	rimeaddr_t msrc;
#if DEBUG_H
	uint16_t debug_time;
#endif
	uint8_t num_blocks;

	uint8_t block_data[];
} __attribute__ ((packed));

/**
 * \brief generates the bundle struct from raw data
 * \param buffer pointer to the buffer with raw data
 * \param size size of raw data
 * \return Pointer to the MMEM struct containing the bundle
 */
struct mmem * bundle_recover_bundle(uint8_t * buffer, int size);

/**
 * \brief Encodes the bundle to raw data
 * \param bundlemem pointer to the MMEM struct containing the bundle
 * \param buffer pointer to a buffer
 * \param max_len Size of the buffer
 * \return The number of bytes that were written to buf
 */
int bundle_encode_bundle(struct mmem * bundlemem, uint8_t * buffer, int max_len);

/**
 * \brief sets an attribute of a bundle
 * \param bundlemem pointer to the MMEM struct containing bundle
 * \param attr attribute to be set
 * \param val pointer to the value to be set
 * \return length of the set value on success or 0 on error
 */
uint8_t bundle_set_attr(struct mmem *bundlemem, uint8_t attr, uint32_t *val);

/**
 * \brief Gets an attribute of a bundle
 * \param bundlemem pointer to the MMEM struct containing the bundle
 * \param attr attribute to get
 * \param val pointer to the variable where the value will be written
 * \return 1 on success or 0 on error
 */
uint8_t bundle_get_attr(struct mmem *bundlemem, uint8_t attr, uint32_t *val);

/**
 * \brief Get a new bundle structure allocated
 * \return MMEM allocation of the bundle, NULL in case of an error
 */
struct mmem * bundle_create_bundle();

/**
 * \brief free a given MMEM allocation of a bundle struct
 * \param bundlemem the MMEM allocation to free
 *
 * A bit of magic is involved here because we want to also free
 * the bundleslot that this bundle belongs to.
 */
uint16_t bundle_delete_bundle(struct mmem * bundlemem);

/**
 * \brief Add a block to a bundle
 * \param bundlemem pointer to the MMEM allocation of the bundle
 * \param type type of the block
 * \param flags processing flags of the block
 * \param data pointer to the block payload
 * \param d_len length of the block payload
 * \return 1 on success or 0 on error
 */
int bundle_add_block(struct mmem * bundlemem, uint8_t type, uint8_t flags, uint8_t * data, uint8_t d_len);

/**
 * \brief Returns a pointer a bundle block
 * \param bundlemem MMEM allocation of the bundle
 * \param i index of the block. Starts at 0
 * \return the block or NULL on error
 */
struct bundle_block_t * bundle_get_block(struct mmem * bundlemem, uint8_t i);

/**
 * \brief Returns pointer to first bundle block of specific type
 * \param bundlemem MMEM allocation of the bundle
 * \param type type of the block (see bundle.h)
 * \return the block of NULL on error
 */
struct bundle_block_t * bundle_get_block_by_type(struct mmem * bundlemem, uint8_t type);

/**
 * \brief Returns pointer to bundle payload block
 * \param bundlemem MMEM allocation of the bundle
 * \return the block of NULL on error
 */
struct bundle_block_t * bundle_get_payload_block(struct mmem * bundlemem);

/**
 * \brief converts IPN EIDs (uint32_t) into the RIME address
 */
rimeaddr_t convert_eid_to_rime(uint32_t eid);

/**
 * \brief converts a RIME address into an IPN EID
 */
uint32_t convert_rime_to_eid(rimeaddr_t * dest);

/**
 * \brief Decrements the reference counter for a MMEM struct containing a bundle
 * Frees the struct when reference counter is down to 0
 * \return Returns the number of remaining references on the bundle or -1 on error
 */
int bundle_decrement(struct mmem *bundlemem);

/**
 * \brief Increments the reference counter for a MMEM struct containing a bundle
 * \return Returns the number of references on the bundle or -1 on error
 */
int bundle_increment(struct mmem *bundlemem);

#endif
/** @} */
/** @} */
