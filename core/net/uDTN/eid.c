/**
 * \addtogroup agent
 * @{
 */

/**
 * \file
 *
 * \author Wolf-Bastian Pöttner <poettner@ibr.cs.tu-bs.de>
 */

#include <string.h>
#include <stdlib.h>

#include "contiki.h"
#include "logging.h"

#include "agent.h"
#include "sdnv.h"

#include "eid.h"

#define BUFFER_LENGTH 100

int eid_parse_host(char * buffer, uint8_t length, uint32_t * node_id)
{
	/* Do we have an ipn scheme? */
	if( strncmp(buffer, "ipn:", 4) != 0 ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Unknown EID format %s", &buffer[4]);
		return -1;
	}

	/* Extract the node part of the EID */
	*node_id = atoi(&buffer[4]);

	/* FIXME: Strings are not null-terminated, strlen does not work */
	return strlen(buffer);
}

int eid_parse_host_length(uint8_t * buffer, uint8_t length, uint32_t * node_id)
{
	uint32_t eid_length;
	int sdnv_length = 0;
	int ret = 0;

	/* Recover the EID length */
	sdnv_length = sdnv_decode(buffer, length, &eid_length);
	if (sdnv_length < 0) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Unknown EID format %s", &buffer[4]);
		return -1;
	}

	/* Parse the EID */
	ret = eid_parse_host((char *) &buffer[sdnv_length], length - sdnv_length, node_id);
	if( ret < 0 ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Unknown EID format %s", &buffer[4]);
		return -1;
	}

	return sdnv_length + eid_length;
}

int eid_parse_ssp(char * buffer, uint8_t length, uint32_t * node_id, uint32_t * service_id)
{
	char * delimeter = NULL;
	int result;

	/* Go and search for the . delimeter */
	delimeter = strchr(buffer, '.');
	if( delimeter == NULL ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Unknown EID format %s", &buffer[4]);
		return -1;
	}

	/* Make this two seperate strings */
	*delimeter = 0;
	delimeter ++;

	/* Now parse host part */
	*node_id = atoi(buffer);

	/* And service part */
	*service_id = atoi(delimeter);

	result = strlen(buffer) + strlen(delimeter) + 1;

	/* Reconstruct the original string */
	*(delimeter-1) = '.';

	return result;
}

int eid_parse_full(char * buffer, uint8_t length, uint32_t * node_id, uint32_t * service_id)
{
	/* Do we have an ipn scheme? */
	if( strncmp(buffer, "ipn:", 4) != 0 ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Unknown EID format %s", &buffer[4]);
		return -1;
	}

	return 4 + eid_parse_ssp(&buffer[4], length - 4, node_id, service_id);
}


int eid_parse_full_length(uint8_t * buffer, uint8_t length, uint32_t * node_id, uint32_t * service_id)
{
	uint32_t eid_length;
	int sdnv_length = 0;
	int ret = 0;

	/* Recover the EID length */
	sdnv_length = sdnv_decode(buffer, length, &eid_length);
	if (sdnv_length < 0) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Unknown EID format %s", &buffer[4]);
		return -1;
	}

	ret = eid_parse_full((char *) &buffer[sdnv_length], length - sdnv_length, node_id, service_id);
	if( ret < 0 ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Unknown EID format %s", &buffer[4]);
		return -1;
	}

	return sdnv_length + ret;
}

int eid_create_host(uint32_t node_id, char * buffer, uint8_t length)
{
	return sprintf(buffer, "ipn:%lu", node_id);
}

int eid_create_host_length(uint32_t node_id, uint8_t * buffer, uint8_t length)
{
	char string_buffer[BUFFER_LENGTH];
	int ret = 0;
	int eid_length = 0;

	/* Encode EID */
	eid_length = eid_create_host(node_id, string_buffer, BUFFER_LENGTH);
	if( eid_length < 0 ) {
		return eid_length;
	}

	/* Encode length of EID */
	ret = sdnv_encode(eid_length, (uint8_t *) buffer, length);
	if (ret < 0) {
		return -1;
	}

	/* Check if we have enough buffer left */
	if( length - ret < eid_length ) {
		return -1;
	}

	/* And then append the EID string */
	memcpy(buffer + ret, string_buffer, eid_length);

	return ret + eid_length;
}

int eid_create_full(uint32_t node_id, uint32_t service_id, char * buffer, uint8_t length)
{
	return sprintf(buffer, "ipn:%lu.%lu", node_id, service_id);
}

int eid_create_full_length(uint32_t node_id, uint32_t service_id, uint8_t * buffer, uint8_t length)
{
	char string_buffer[BUFFER_LENGTH];
	int ret = 0;
	int eid_length = 0;

	/* Encode EID */
	eid_length = eid_create_full(node_id, service_id, string_buffer, BUFFER_LENGTH);
	if( eid_length < 0 ) {
		return eid_length;
	}

	/* Encode length of EID */
	ret = sdnv_encode(eid_length, (uint8_t *) buffer, length);
	if (ret < 0) {
		return -1;
	}

	/* Check if we have enough buffer left */
	if( length - ret < eid_length ) {
		return -1;
	}

	/* And then append the EID string */
	memcpy(buffer + ret, string_buffer, eid_length);

	return ret + eid_length;
}
