/**
 * \addtogroup agent
 * @{
 */
 
 /**
 * \defgroup delivery Auslieferung von Bündeln
 *
 * @{
 */
 
 /**
 * \file
 *         Headerfile Übergabefunktion an Anwendungen
 *
 */
 
#ifndef DELIVERY_H
#define DELIVERY_H

/**
*   \brief Übergibt Nutzdaten eines Bündels an Anwendung
*
*   \param bundle Das empfangene Bündel
*   \param registration der empfangenden prozesses
*/
void deliver_bundle(bundle_t *bundle, struct registration *n);

#endif

/** @} */
/** @} */
