/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \brief DTN App Support file
 * \author Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef DTN_APPS_H
#define DTN_APPS_H

#include "contiki.h"

extern const struct dtn_app * dtn_apps[];

struct dtn_app {
	/**
	 * \brief A string name of the dtn_app (only for debugging)
	 */
	char * name;

	/**
	 * \brief Initializes a DTN app
	 *
	 * \returns 1 in success, 0 otherweise
	 */
	int (* init)(void);

	/**
	 * \brief Parse IPND service block
	 *
	 * \param eid EID the discovery was received from
	 * \param name Pointer to the incoming packet where the name is stored (will usually be a string)
	 * \param name_length Length of the name field
	 * \param params Pointer to the parameters of the IPND service block
	 * \param params_length Length of the parameters field
	 *
	 */
	void (* parse_ipnd_service_block)(uint32_t eid, uint8_t * name, uint8_t name_length, uint8_t * params, uint8_t params_length);

	/**
	 * \brief Add IPND service blocks, returns number of added blocks
	 *
	 * \param ipnd_buffer Pointer to the begin of the outgoing IPND buffer (offset contains the position to write)
	 * \param buffer_length Total length of the buffer
	 * \param offset Absolute offset within the total buffer (may never be larger than buffer_length)
	 *
	 * \returns number of added service blocks
	 */
	int (* add_ipnd_service_block)(uint8_t * ipnd_buffer, int buffer_length, int * offset);
};

void dtn_apps_start();

#endif /* DTN_APPS_H */

/** @} */
