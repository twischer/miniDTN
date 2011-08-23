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

/**
*   \brief Struct for service registration
*
*/
struct registration_api {
	uint32_t app_id;
	uint8_t status:1;
	struct process *application_process;
};


#endif
/** @} */
