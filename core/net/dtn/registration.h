/**
 * \addtogroup bprocess 
 * @{
 */
 
 /**
 * \file
 *         Headerfile Registrierung
 *
 */
#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdint.h>

#include "lib/list.h"
#include "net/dtn/API_registration.h"

#ifndef APP_ACTIVE
#define APP_ACTIVE	1
#endif
#ifndef APP_PASSIVE
#define APP_PASSIVE	0
#endif

list_t reg_list;


/**
*   \brief structure of registration
*/
struct registration {
	struct registration *next;
	uint32_t node_id;
	uint32_t app_id;
	uint8_t status:1;
	struct process *application_process;
};

struct process *registration_get_process(uint32_t app_id);


/**
*   \brief called by agent at startup
*/
void registration_init(void);


/**
*   \brief registers a new service
*
*   \param app_id id of service
*   \param process pointer to service
*
*   \return 1 on succes, 0 on error
*/
int registration_new_app(uint32_t app_id, struct process *application_process);

/**
*   \brief delets registration of service 
*
*   \param app_id service id
*/
void registration_remove_app(uint32_t app_id) ;

/**
*   \brief sets state of service active
*
*   \param app_id id of service
*
*/
int registration_set_active(uint32_t app_id);

/**
*   \brief sets state of service passiv
*
*   \param app_id id of service
*
*/
int registration_set_passive(uint32_t app_id) ;

/**
*
*   \param app_id id of service
*
*   \return state of service
*/
int registration_return_status(uint32_t app_id);

#endif
/** @} */
