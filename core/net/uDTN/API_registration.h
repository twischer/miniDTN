/**
 * \addtogroup api
 * @{
 */
   /**
 * \file
 *      this file defines the service registration struct 
 */
#ifndef API_REGISTRATION_H
#define API_REGISTRATION_H

#ifndef APP_ACTIVE
#define APP_ACTIVE	1
#endif

#ifndef APP_PASSIVE
#define APP_PASSIVE	0
#endif

/**
*   \brief Struct for service registration
*
*/
struct registration_api {
	uint32_t app_id;
	uint8_t status:1;
	uint32_t node_id;
	struct process *application_process;
};


#endif
/** @} */
