/**
 * \addtogroup api
 * @{
 */
   /**
 * \file
 *        
 */
#ifndef API_REGISTRATION_H
#define API_REGISTRATION_H

/**
*   \brief Struktur mit der Anwendungen sich beim Bundle Agent registrieren, können
*
*/
struct registration_api {
	uint32_t app_id;
	uint8_t status:1;
	struct process *application_process;
};


#endif
/** @} */
