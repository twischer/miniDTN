/**
 * \addtogroup api
 * @{
 */
   /**
 * \file
 *        
 */
#ifndef API_EVENTS_H
#define API_EVENTS_H

#include "contiki.h"
/**
*   \brief Events um Anwendung zu Registrieren
 */
process_event_t dtn_application_registration_event;
/**
*   \brief Events um regstrierte Anwendung zu dergistrieren 
*/
process_event_t dtn_application_remove_event;
/**
*   \brief Events um den Status einer Anwendung zu ändern
*/
process_event_t dtn_application_status_event;
/**
*   \brief Events ein erstelltes Bündel zu Senden
*/
process_event_t dtn_send_bundle_event;


/**
*   \brief Event mit dem der Bundle Agent empfangene Daten an die Applikation übergibt
 *
*/
process_event_t submit_data_to_application_event;


#endif
/** @} */

