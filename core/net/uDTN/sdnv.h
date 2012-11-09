/**
 * \addtogroup agent
 * @{
 */

 /**
 * \defgroup sdnv SDNV
 *
 * @{
 */

/**
 * \file 
 * Headerfile for sdnv functions
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 */
#ifndef __SDNV_H__
#define __SDNV_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "contiki.h"

typedef uint8_t *sdnv_t;

/**
* \brief encodes an uint32 value in sdnv
* \param val value to be encoded
* \param bp pointer to sdnv 
* \param size of sdnv
* \return length of sndv
*/
int sdnv_encode(uint32_t val, uint8_t* bp, size_t len);
/** 
* \brief calculates the length needed to encode an uint32 value in sdnv
* \param val value to be encoded
* \return length of sndv
*/
size_t sdnv_encoding_len(uint32_t val);
/**
* \brief decodes a sdnv to an uint32 value
* \param bp pointer to sdnv
* \param len length of sdnv
* \param val pointer to uint32 value
* \return length of sndv
*/
int sdnv_decode(const uint8_t* bp, size_t len, uint32_t* val);
/**
* \brief calculates the length of a sdnv
* \param bp pointer to sdnv
* \return length of sndv
*/
size_t sdnv_len(const uint8_t* bp);

#define sdnv_decode2(a,b) sdnv_decode(a,sdnv_len(a),b)

#endif
/** @} */
/** @} */
