/**
 * \addtogroup agent
 * @{
 */
 
 /**
 * \defgroup registration Registrierung von Anwendungen
 *
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

/**
* \name Definitionen des Status einer Anwendung
*   @{
*/
#ifndef APP_ACTIVE
#define APP_ACTIVE	1
#endif
#ifndef APP_PASSIVE
#define APP_PASSIVE	0
#endif
/**    @} */

/**
*   \brief Liste mit den registrierten Anwendungen
*/
list_t reg_list;


/**
*   \brief Struktur eines Listeneintrags
*/
struct registration {
	struct registration *next;
	uint32_t node_id;
	uint32_t app_id;
	uint8_t status:1;
	struct process *application_process;
};

/**
*   \brief Initialisierung der Registrierung
*
*   Wird vom Bundle Protocol Agent aus aufgerufen
*/
void registration_init(void);


/**
*   \brief Registrierung einer neuen Anwendung 
*
*   \param name Name der Anwendung
*   \param process Zeiger auf die Prozess Struktur der zu registrierenden Anwendung
*
*    Diese Informationen werden von der Anwendung in einer in der API definierten Struktur mit einem
*    Event übergeben
*
*   \return 1 wenn erfolgreich, 0 wenn Anwendung bereits registriert und
*    -1 sollte eine Registrierung nicht möglich sein
*/
int registration_new_app(uint32_t app_id, struct process *application_process);

/**
*   \brief Entferne Registrierung einer Anwendung
*
*   \param name Name der Anwendung
*/
void registration_remove_app(uint32_t app_id) ;

/**
*   \brief Setze Status einer Anwendung auf aktiv
*
*   \param name Name der Anwendung
*
*   \return status der Anwendung (1 wenn activ, 0 if passiv)
*    oder -1 wenn Anwendung nicht registriert
*/
int registration_set_active(uint32_t app_id);

/**
*   \brief Setze Status einer Anwendung auf passiv
*
*   \param name Name der Anwendung
*
*   \return status der Anwendung (1 wenn activ, 0 if passiv)
*    oder -1 wenn Anwendung nicht registriert
*/
int registration_set_passive(uint32_t app_id) ;

/**
*   \brief Gibt den Status einer Anwendung aus
*
*   \param name Name der Anwendung
*
*   \return status der Anwendung (1 wenn activ, 0 if passiv)
*    oder -1 wenn Anwendung nicht registriert
*/
int registration_return_status(uint32_t app_id);

#endif
/** @} */
/** @} */
