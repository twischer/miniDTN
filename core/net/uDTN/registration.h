/**
 * \addtogroup bprocess 
 * @{
 */
 
/**
 * \file
 * \brief Handle Service Registrations
 *
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdint.h>

#include "contiki.h"

#include "lib/list.h"

list_t reg_list;

/**
 * \brief structure of registration
 */
struct registration {
	struct registration * next;
	uint32_t node_id;
	uint32_t app_id;
	uint8_t status:1;
	uint8_t busy:1;
	struct process * application_process;
};

/**
 * \brief called by agent at startup
 */
void registration_init(void);

/**
 * \brief registers a new service
 *
 * \param app_id id of service
 * \param application_process pointer to service
 * \param node_id Node ID
 *
 * \return 1 on success, 0 on error
 */
int registration_new_application(uint32_t app_id, struct process * application_process, uint32_t node_id);

/**
 * \brief deletes registration of service
 *
 * \param app_id service id
 * \param node_id Node ID
 */
void registration_remove_application(uint32_t app_id, uint32_t node_id);

/**
 * \brief sets state of service active
 *
 * \param app_id id of service
 * \param node_id Node ID
 *
 * \return New status when successful, -1 on error
 */
int registration_set_active(uint32_t app_id, uint32_t node_id);

/**
 * \brief sets state of service passive
 *
 * \param app_id id of service
 * \param node_id Node ID
 *
 * \return New status when successful, -1 on error
 */
int registration_set_passive(uint32_t app_id, uint32_t node_id);

/**
 * \brief Returns the status of a registred service
 *
 * \param app_id id of service
 * \param node_id Node ID
 *
 * \return state of service
 */
int registration_return_status(uint32_t app_id, uint32_t node_id);

/**
 * \brief returns the process struct for a specific registration
 *
 * \param app_id Service identifier
 * \param node_id Node ID
 *
 * \return pointer to process struct
 */
struct process * registration_get_process(uint32_t app_id, uint32_t node_id);

/**
 * \brief provides the endpoint id of a registred process (if any)
 *
 * \param application_process Pointer to the process
 *
 * \return app_id, 0xFFFF on error
 */
uint32_t registration_get_application_id(struct process * application_process);

/**
 * \brief Checks whether the bundle is for a local process
 *
 * \param app_id Service Identifier
 * \param node_id Node Identifier
 *
 * \return 1 on local, 0 otherwise
 *
 */
uint8_t registration_is_local(uint32_t app_id, uint32_t node_id);

#endif
/** @} */
