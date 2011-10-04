/**
 * \addtogroup agent
 * @{
 */

 /**
 * \defgroup bundle Bundle 
 *
 * @{
 */

/**
 * \file 
 * this file defines bundle stucture
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 */
#include <stdint.h>
#include "mmem.h"
#include "net/rime/rimeaddr.h"
//#include "net/dtn/bundle.h"
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

//payload block defines
#define DATA 17

#define OFFSET 0
#define STATE 1

#define DEBUG_H 1

/**
* \brief this struct defines the bundel for internal prcessing
*/
struct bundle_t{
	char offset_tab[18][2];
	uint8_t size;
	uint8_t custody;
	uint8_t del_reason;
	uint32_t rec_time;
	uint16_t bundle_num;
	uint32_t lifetime;
	uint32_t flags;
	rimeaddr_t msrc;
	struct mmem mem;
	// struct mmem *mem;
#if DEBUG_H
	uint16_t debug_time;
#endif
};

/**
* \brief generates the bundle struct from row data
* \param bundle_t pointer to empty bundle struct
* \param mmem pointer to mmem struct with row data
* \param size size of row data
* \return 1 on success or 0 if something fails 
*/
uint8_t recover_bundel(struct bundle_t *bundle,struct mmem *mem,int size);
/**
* \brief stets an attribute of a bundle
* \param bundle_t pointer to bundle
* \param attr attribute to be set
* \param val pointer to the value to be set 
* \return length of the seted value on success or 0 on error
*/
uint8_t set_attr(struct bundle_t *bundle, uint8_t attr, uint32_t *val);
/**
* \brief creates a new bundle and allocates the minimum needed memory
* \param bundle_t pointer to an empty bundle struct 
* \return 1 on success or 0 on error
*/
uint8_t create_bundle(struct bundle_t *bundle);
/**
* \brief delets a bundle struct and frees mmem parts
* \param bundle_t poiter to bundle
* \return 1 on success or 0 on error
*/
uint16_t delete_bundle(struct bundle_t *bundel);
/**
* \brief adds a block to an existing bundle
* \param bundle_t poiter to bundle
* \param type type of block
* \param flags processing flags
* \param data pointer to block data
* \param d_len length of data
* \return 1 on success or 0 on error
*/
uint8_t add_block(struct bundle_t *bundle, uint8_t type, uint8_t flags, uint8_t *data, uint8_t d_len);
#endif
/** @} */
/** @} */
