/**
 * \addtogroup agent
 * @{
 */
 
 /**
 * \defgroup dispatching Abfertigung empfangener Bündel
 *
 * @{
 */
 
 /**
 * \file
 *         Headerfile für Funktion zur Abfertig
 *
 */
#ifndef DISPATCHING_H
#define DISPATCHING_H

/**
*   \brief Abfertigung des Bündels, Eintscheidung ob Weiterleitung oder Auslieferung an Anwendungen
*
*   \param  bundle das zu verarbeitende Bündel
*/
void dispatch_bundle(bundle_t *bundle);

#endif

/** @} */
/** @} */
