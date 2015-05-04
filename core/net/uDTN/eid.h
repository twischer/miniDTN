/**
 * \addtogroup agent
 * @{
 */

/**
 * \defgroup eid EID Representation and Helper Methods
 *
 * @{
 */

/**
 * \file
 * \brief this file defines the memory representation of an EID and provides methods for construction and parsing
 *
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdint.h>

#include "contiki.h"

#ifndef __EID_H__
#define __EID_H__

/**
 * \brief Parses an incoming string and fills out the EID parts
 *
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 * \param node_id Pointer to where node id is stored
 *
 * \return Returns length of parsed EID on success or -1 on error
 */
int eid_parse_host(char * buffer, uint8_t length, uint32_t * node_id);

/**
 * \brief Parses an byte buffer that contains the SDNV length and an EID string and fills out the EID parts
 *
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 * \param node_id Pointer to where node id is stored
 *
 * \return Returns length of parsed EID on success or -1 on error
 */
int eid_parse_host_length(uint8_t * buffer, uint8_t length, uint32_t * node_id);

/**
 * \brief Parses an incoming string for the scheme specific part (SSP) and fills out the EID parts
 *
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 * \param node_id Pointer to where node id is stored
 * \param service_id Pointer to where the service id is stored
 *
 * \return Returns length of parsed EID on success or -1 on error
 */
int eid_parse_ssp(char * buffer, uint8_t length, uint32_t * node_id, uint32_t * service_id);

/**
 * \brief Parses an incoming string and fills out the EID parts
 *
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 * \param node_id Pointer to where node id is stored
 * \param service_id Pointer to where the service id is stored
 *
 * \return Returns length of parsed EID on success or -1 on error
 */
int eid_parse_full(char * buffer, uint8_t length, uint32_t * node_id, uint32_t * service_id);

/**
 * \brief Parses an byte buffer that contains the SDNV length and an EID string and fills out the EID parts
 *
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 * \param node_id Pointer to where node id is stored
 * \param service_id Pointer to where the service id is stored
 *
 * \return Returns length of parsed EID on success or -1 on error
 */
int eid_parse_full_length(uint8_t * buffer, uint8_t length, uint32_t * node_id, uint32_t * service_id);

/**
 * \brief Converts the EID struct into a string representation
 *
 * \param eid Pointer to the EID struct
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 *
 * \return Returns length of string on success or -1 on error
 */
int eid_create_host(uint32_t node_id, char * buffer, uint8_t length);

/**
 * \brief Converts the EID struct into a string representation prefixed with the SDNV-encoded length
 *
 * \param eid Pointer to the EID struct
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 *
 * \return Returns length of string on success or -1 on error
 */
int eid_create_host_length(uint32_t node_id, uint8_t * buffer, uint8_t length);

/**
 * \brief Converts the EID struct into a string representation
 *
 * \param eid Pointer to the EID struct
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 *
 * \return Returns length of string on success or -1 on error
 */
int eid_create_full(uint32_t node_id, uint32_t service_id, char * buffer, uint8_t length);

/**
 * \brief Converts the EID struct into a string representation prefixed with the SDNV-encoded length
 *
 * \param eid Pointer to the EID struct
 * \param buffer Pointer to the string buffer
 * \param length Length of the string buffer
 *
 * \return Returns length of string on success or -1 on error
 */
int eid_create_full_length(uint32_t node_id, uint32_t service_id, uint8_t * buffer, uint8_t length);

#endif
/** @} */
/** @} */
