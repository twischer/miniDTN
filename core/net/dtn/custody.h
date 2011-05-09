/**
 * \addtogroup agent
 * @{
 */
 
 /**
 * \defgroup custody Custody Transfer
 *
 * @{
 */
 
 /**
 * \file
 *         Headerfile für Funktionen zur persistenten Speicherung von Bündeln, sowie Custody Transfer Funktionen
 *
 */
 
#ifndef CUSTODY_H
#define CUSTODY_H

#include "API/bundle.h"
#include "contiki.h"

/**
*   \brief Rentransmission Timer für gespeicherte Bündel
*/
static struct etimer custody_etimer;


/**
*   \brief Initialisierungsfunktion für das Custody Modul
*
*   Wird vom Bundle Protocol beim Start ausgeführt
*/
void custody_init(void);

/**
*   \brief Setzt den aktuellen Knoten als Custodian im Bündel-Dictionary
*
*   \param bundle Das Bündel, im dem der neue Custodian eingetragen werden soll
*/
void custody_set_custodian(bundle_t *bundle);

/**
*   \brief Gibt den Inhalt der Custody Liste auf der Konsole aus -> Debuggingfunktion
*/
void check_custody_list(void);

/**
*   \brief Liest die im Flash gespeicherte Custody Liste ein
*
*   Wird von der Init-Funktion aufgerufen
*
*   \return Anzahl gelesener Bytes
*/
int read_custody_list(void);

/**
*   \brief Überprüft ob genug Speicher für ein weiteres Bundle  frei ist
*
*   \return 0 wenn kein Platz, 1 wenn noch Platz ist
*/
int custody_check_memory(void);

/**
*   \brief Speichern eins Bündels im Flash-Speicher
*
*   \param bundle Das zu speichernde Bündel
*
*   \return -1 wenn speichern fehlgeschlagen,  0 wenn Bündel schon verhanden, oder die Länge
*   der gespeicherten Daten wenn erfolgreich
*/
int custody_save_bundle(bundle_t *bundle);

/**
*   \brief Auslesen eins Bündels aus Flash-Speicher
*
*   Es wird immer das Bündel ausgelesen was als nächstes gesendet werden muss
*
*   \param bundle Struktur in der Bündel eingelesen werden soll
*
*   \return -1 wenn lesen fehlgeschlagen oder die Länge der gelesenen Daten, wenn erfolgreich
*/
int custody_read_bundle(bundle_t *bundle);


/**
*   \brief Überprüft ob ein Bündel bereits im Speicher ist
*
*   \param eid EID des zu überprüfenden Bündels
*   \param timestamp Zeitstempel des zu überprüfenden Bündels
*   \param timestamp_seq Zeitstempel-Sequenznummer des zu überprüfenden Bündels
*
*   \return 0 wenn Bündel noch nicht im Speicher, 1 wenn schon vorhanden
*/
int custody_check_redundancy(uint32_t src_node, uint32_t src_app, uint32_t timestamp, uint32_t timestamp_seq);

/**
*   \brief Entfernt Bündel aus Speicher
*
*   \param eid EID des zu entfernenden Bündels
*   \param timestamp Zeitstempel des zu entfernenden Bündels
*   \param timestamp_seq Zeitstempel-Sequenznummer des zu entfernenden Bündels
*
*   \return -1 wenn Datei nicht gefunden, sonst Anzahl der im Flash freigewordenen Bytes
*/
int custody_remove_bundle(uint32_t src_node, uint32_t src_app, uint32_t timestamp, uint32_t timestamp_seq);


#endif


/** @} */
/** @} */
