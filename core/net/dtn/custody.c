/**
 * \addtogroup custody
 * @{
 */

/**
 * \file
 *          Funktionen zur persistenten Speicherung von Bündeln, sowie Custody Transfer Funktionen
 *
 */
 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "net/dtn/agent.h"
#include "net/dtn/bundle.h"
#include "net/dtn/storage.h"

#include "cfs.h"
#include "cfs-coffee.h"
#include "API/DTN-block-types.h"
#include "custody.h"
#include "dtn-config.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "clock.h"
#include "dictionary.h"
#include "bundlebuf.h"

#ifndef MAXIMUM_EID_LEN
#define MAXIMUM_EID_LEN	128
#endif

#define MAX_CUSTODY_ENTRIES 10

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint16_t CUSTODY_LIST_ENTRY_SIZE;

/* Element eines Custody Listen Eintrags */
struct custody_element{
	
	struct registration *next;
	uint16_t bundle_num;
	uint8_t length;
	uint32_t custody_time;
	uint32_t retransmit;
	uint32_t src_node;
	uint32_t src_app;
	uint32_t timestamp;
	uint32_t timestamp_seq;	
};

LIST(custody_list);
MEMB(custody_mem, struct custody_element, MAX_CUSTODY_ENTRIES);

/**
*   \brief Berechnet die Länge eines Bündels
*
*   \param bundle das Bündel, dessen Länge berechnet werden soll
*
*   \return Länge in Bytes
*/
uint8_t length_of_bundle(bundle_t *bundle) {
	return bundle->size;
}


void custody_set_custodian(bundle_t *bundle) {
	set_attr(bundle, CUST_NODE, dtn_node_id);	
}	

/* speichert custody listen eintrag in einem Puffer zur speicherung im Flash */
/**
*   \brief Speichert ein custody Listenelement in einem Puffer
*
*  Dient zum Speichern der Custody Liste aus dem persistenten Speicher
*
*   \param buffer Puffer in dem das Element geschrieben werden soll
*   \param custody_element Listenelement das gespeichert werden soll
*
*   \return Länge des Listenelements
*/
int custody_list_to_buffer(uint8_t *buffer, struct custody_element *n) {
	
	uint8_t i = 0, count = 0;
	
	while(i < 15) {
		
		buffer[count] =(uint8_t) n->file[i];
		if(n->file[i] == '\0') {
			count += 15-i;
			break;
		}
		count++;
		i++;
	}
	buffer[count] = n->length & 0xff;
	count++;
	buffer[count] = (n->length >> 8) & 0xff;
	count++;
	
	memcpy(&buffer[count], &n->custody_time, 4);
	count += 4;
	
	memcpy(&buffer[count], &n->retransmit, 4);
	count += 4;
	
	i = 0;
	while(i < MAX_EID_LEN) {
		buffer[count] =(uint8_t) n->eid[i];
		if(n->eid[i] == '\0') {
			count += MAX_EID_LEN-i;
			break;
		}
		count++;
		i++;
	}
	
	memcpy(&buffer[count], &n->timestamp, 4);
	count += 4;
	
	buffer[count] = n->timestamp_seq;
	count++;

	return count;
}

/* Liest custody listen element aus puffer ein */
/**
*   \brief Liest ein custody Listenelement aus einem Puffer ein
*
*  Dient zum Laden der Custody Liste im persistenten Speicher
*
*   \param buffer Puffer der das Element enthält
*   \param custody_element Listenelement das geladen werden soll
*
*   \return Länge des Elements
*/
int custody_buffer_to_list (uint8_t *buffer, struct custody_element *n) {
	
	uint8_t count = 0, i = 0;
	
	while(i < 15) {
		
		n->file[i] = (char) buffer[count];
		if(n->file[i] == '\0') {
			count += 15-i;
			break;
		}
		count++;
		i++;
	}
	
	PRINTF("CUSTODY LIST: custody buffer to file: %s \n", n->file);
	
	n->length = buffer[count] + (buffer[count+1] << 8);
	count += 2;
	
	memcpy(&n->custody_time, &buffer[count], 4);
	count += 4;
	
	memcpy(&n->retransmit, &buffer[count], 4);
	count += 4;
	
	i = 0;
	while(i < MAX_EID_LEN) {
		
		n->eid[i] =(char) buffer[count];
		if(n->eid[i] == '\0'){
			count += MAX_EID_LEN-i;
			break;
		}
		count++;
		i++;
	}
	
	PRINTF("custody eid: %s \n", n->eid);
	
	memcpy(&n->timestamp, &buffer[count], 4);
	count += 4;

	n->timestamp_seq = buffer[count];
	count++;
	
	return count;	
}

/** \brief just a debugging function */
void check_custody_list(void) {
#if DEBUG	
	struct custody_element *n;
	
	for(n = list_head(custody_list); n != NULL; n = list_item_next(n)) {
		
		PRINTF("CUSTODY: Custody Filename: %s \n", n->file);
		PRINTF("CUSTODY: Custody EID: %s \n", n->eid);
		PRINTF("CUSTODY: Bundle Length: %i \n", n->length);
		PRINTF("CUSTODY: Time %i \n", clock_time());
		PRINTF("CUSTODY: Retransmit at: %i \n", n->retransmit);
	}
	if((n = list_head(custody_list)) == NULL) 
		PRINTF("No Custody Elements\n");
#endif
}
	
/* speichert custody liste im flash */
/**
*   \brief Speichert Custody Liste im Flash
*
*  Dient zum Speichern der Custody Liste im persistenten Speicher
*
*   \return -1 wenn keine Datei angelegt werden kann, 1 wenn Speichern erfolgreich
*/
int save_custody_list(void) {
	
	struct custody_element *n;
	int fd, written_data;
	uint8_t buffer[CUSTODY_LIST_ENTRY_SIZE];
	memset(&buffer, 0xff, CUSTODY_LIST_ENTRY_SIZE);
	
	/* entferne datei der alten liste und lege neue an */
	cfs_remove("custody_list.sv");
	cfs_coffee_reserve("custody_list.sv", MAX_CUSTODY_ENTRIES*CUSTODY_LIST_ENTRY_SIZE); 
	
	fd = cfs_open("custody_list.sv", CFS_WRITE);
	if(fd == -1) {
		
		PRINTF("CUSTODY LIST: cant open list \n");
		return -1;
	}
	
	/* breche speichern ab, wenn liste leer */
	if(list_head(custody_list) == NULL) {
		
		cfs_close(fd);		
		return 1;
	}	
	
	/* schreibe jedes element erst in puffer und von da aus in datei */
	for(n = list_head(custody_list); n != NULL; n = list_item_next(n)) {
		
		custody_list_to_buffer(buffer, n);
		written_data = cfs_write(fd, buffer,  CUSTODY_LIST_ENTRY_SIZE);
		if(written_data != CUSTODY_LIST_ENTRY_SIZE)
			PRINTF("CUSTODY LIST: error while writing \n");
	}
	cfs_close(fd);
	return 1;
}

/* liest custody liste aus datei */
/**
*   \brief Läd Custody Liste aus Flash-Speicher
*
*   \return -1 wenn keine Datei gefunden werden kann, sonst Länge der Datei
*/
int read_custody_list(void) {
	
	struct custody_element n;
	int fd;
	uint8_t buffer[CUSTODY_LIST_ENTRY_SIZE], read_data;
	uint16_t total_data;
	
	memset(buffer, 0, CUSTODY_LIST_ENTRY_SIZE);
	
	fd = cfs_open("custody_list.sv", CFS_READ);
	if(fd == -1)
		return -1;
	
	/* die größe eines listeneintrags aus der datei aus */
	read_data = cfs_read(fd, buffer,  CUSTODY_LIST_ENTRY_SIZE);
	PRINTF("CUSTODY LIST: read data: %i \n", read_data);
	total_data = read_data;
	
	/* liest solange aus bis, kein entrag mehr in datei ist */
	while(read_data ==  CUSTODY_LIST_ENTRY_SIZE) {
		
		struct custody_element *list_element;
		
		list_element = memb_alloc(&custody_mem);
		custody_buffer_to_list(buffer, &n);
		
		/* fügt eintrag zu liste hinzu */
		if(list_element != NULL) {
			
			list_add(custody_list, list_element);
			strcpy(list_element->eid, n.eid);
			list_element->timestamp  = n.timestamp;	
			list_element->timestamp_seq = n.timestamp_seq;
			list_element->length = n.length;
			list_element->retransmit = n.retransmit;
			list_element->custody_time =n.custody_time;
			strncpy(list_element->file, n.file, 15);
		}
		
		/* leert den lesepuffer */
		memset(buffer, 0, CUSTODY_LIST_ENTRY_SIZE);
		read_data = cfs_read(fd, buffer,  CUSTODY_LIST_ENTRY_SIZE);
		total_data += read_data;
		PRINTF("CUSTODY LIST: read data: %i \n", read_data);
	}
	cfs_close(fd);
	return total_data;
}

void custody_init(void) {
	
	/* initialisiert speicher für liste und liste selbst */
	memb_init(&custody_mem);
	list_init(custody_list);
	
	/* bestimmte größe eines Listeneintrags */
	CUSTODY_LIST_ENTRY_SIZE = sizeof(struct custody_element) - sizeof(struct custody_element *);
	
	PRINTF("CUSTODY LIST: entry size: %i \n", CUSTODY_LIST_ENTRY_SIZE);
	read_custody_list();
}

int custody_check_memory(void) {
	
	uint8_t length;
	length = list_length(custody_list);
	
	if(length >= MAX_CUSTODY_ENTRIES)
		return 0;
	else
		return 1;
}

/* setze retransmission timer */
/**
*   \brief Aktualisiert den Custody Timer, der das Senden eines Bündels aus dem Speicher veranlasst
*/
void custody_set_custody_etimer(void) {
	
	struct custody_element *n;
	uint32_t min_retransmit_time = 0xffffffff, time, interval;
	time = clock_time();
	
	/* breche ab wenn kein Element in Liste*/
	if((n = list_head(custody_list)) == NULL) {

		etimer_stop(&custody_etimer);
		return;
	}
	
	/* suche nach Bündel mit frühster Retransmission Time */
	for(n = list_head(custody_list); n != NULL; n = list_item_next(n)) {
		
		/* Sollte Sendezeitpunkt in der Vergangenheit liegen, setzte in auf 10 sec in Zunkuft */
		if(time > n->retransmit)
				n->retransmit = time + 10*CLOCK_SECOND;
		
		if(n->retransmit < min_retransmit_time)
			min_retransmit_time = n->retransmit;
	}
	
	/* berechnet verbliebene Zeit für nächste Sendewiederholung*/	
	interval = min_retransmit_time - time;	
	PRINTF("CUSTODY: Next retransmit: %i \n", interval);
	
	etimer_set(&custody_etimer, interval);
}


int custody_save_bundle(bundle_t *bundle) {
	
	
	int fd, len, error;
	uint32_t written_data, time;
	char file[15], eid[128];
	struct custody_element *n;
	uint8_t nr_ssp, nr_scheme;
	
	time = clock_time();
	len = length_of_bundle(bundle);
	PRINTF("CUSTODY WRITE: Bundle Length %i \n", len);
	
	/* erstelle Datei mit akuteller Zeit als dateiname */
	longtoascii(time, file);	
	strcat(file, ".sv");	
		
	uint8_t *custody_buffer;
	
	/* Estelle EID aus Scheme und SSP */
	nr_scheme = dictionary_get_element(bundle, bundle->bundle_primary_block.src_scheme_offset);
	nr_ssp = dictionary_get_element(bundle, bundle->bundle_primary_block.src_ssp_offset);
	strcpy(eid, bundle->bundle_primary_block.dictionary[nr_scheme]);;
	strcat(eid, ":");
	strcat(eid, bundle->bundle_primary_block.dictionary[nr_ssp]);
	
	/* Überprüfe ob Bündel schon in gespeichert ist*/	
	for(n = list_head(custody_list); n != NULL; n = list_item_next(n)) {
		
		if(strcmp(eid, n->eid)  == 0 && 		
			bundle->bundle_primary_block.timestamp == n->timestamp && 
			bundle->bundle_primary_block.timestamp_seq == n->timestamp_seq ) {
				PRINTF("CUSTODY WRITE: bundle in custody\n");
				return 0;
		}
	}
	
	/* füge bündel zu liste hinzu, wenn es noch nicht vorhanden ist */
	if(n == NULL) {
		
		n = memb_alloc(&custody_mem);
		if(n != NULL) {
			
			list_add(custody_list, n);
			strcpy(n->eid, eid);
			n->timestamp  = bundle->bundle_primary_block.timestamp;	
			n->timestamp_seq = bundle->bundle_primary_block.timestamp_seq;
			n->length = len;
			n->retransmit = time + DEFAULT_RETRANSMIT_TIME*CLOCK_SECOND;
			n->custody_time = clock_time();
			strncpy(n->file, file, 15);
		}
		if(n == NULL) 
			return -1;
	}
	/* speichere aktuelle custody liste im flash */
	save_custody_list();
	
	/* lege datei an */
	error = cfs_coffee_reserve(file, len+1); 
	
	if(error == -1)
		PRINTF("CUSTODY WRITE: cant reserve memory \n");
	
	fd = cfs_open(file, CFS_WRITE);
		
	
	if(fd == -1) {
		PRINTF("CUSTODY WRITE: cant write file \n");
		list_remove(custody_list, n);
		memb_free(&custody_mem, n);
		save_custody_list();
		return -1;
	}
	
	
	/* Speichere Bündel in Bundlebuf */
	bundlebuf_copyfrom(bundle);
	custody_buffer = bundlebuf_getptr();
	custody_buffer[len] = 0xff;
	
	/* schreibe bündel in datei und leere den puffer */
	written_data = cfs_write(fd, custody_buffer, len+1);
	bundlebuf_clear();
	
	/* überprüfe ob alles gespeichert wurde */
	if(written_data != (len+1)) {
		
		list_remove(custody_list, n);
		memb_free(&custody_mem, n);
		cfs_close(fd);
		cfs_remove(file);
		return -1;	
	}
	custody_set_custody_etimer();
	cfs_close(fd);
	return written_data;
}


int custody_read_bundle(bundle_t *return_bundle) {
	
	int fd;
	uint32_t read_data;
	struct custody_element *n;
	uint32_t time = clock_time();
		
	/* suche das zu lesende Bündel anhand der retransmission time*/
	for(n = list_head(custody_list); n != NULL; n = list_item_next(n)) {
		
		if(n->retransmit <= time) {
			
			/* aktualisiere Rentransmission time */
			n->retransmit += DEFAULT_RETRANSMIT_TIME*CLOCK_SECOND;
			break;
		}
	}
	
	/* akutalisiere Retransmission timer */
	custody_set_custody_etimer();
	
	if(n == NULL)
		return 0;
	
	/* öffne datei und lese bündel */
	fd = cfs_open(n->file, CFS_READ);
	if(fd == -1) {
		
		PRINTF("CUSTODY READ: can't open file \n");
		return -1;
	}
	uint8_t *buffer  = bundlebuf_getptr();
	read_data = cfs_read(fd, buffer, n->length);
	
	if(read_data != n->length) {
		
		PRINTF("CUSTODY READ: error reading file, length: %i \n", read_data);
		cfs_close(fd);
		return -1;
	}
	
	bundlebuf_copyto(return_bundle);
	return_bundle->bundle_primary_block.lifetime -= clock_time() - n->custody_time;
	cfs_close(fd);
	return read_data;
}

int custody_check_redundancy(char *eid, uint32_t timestamp, uint8_t timestamp_seq) {
	
	struct custody_element *n;
	
	for(n = list_head(custody_list); n != NULL; n = list_item_next(n)) {
				
		if(strcmp(eid, n->eid) == 0 && timestamp == n->timestamp && timestamp_seq == n->timestamp_seq) {
			
			return 1;
		}
	}
	return 0;
}
	

int custody_remove_bundle(char *eid, uint32_t timestamp, uint8_t timestamp_seq) {
	
	struct custody_element *n;
	int file_removed = -1;
	
	/* suche und entferne Bündel */
	for(n = list_head(custody_list); n != NULL; n = list_item_next(n)) {
				
		if(strcmp(eid, n->eid) == 0 && timestamp == n->timestamp && timestamp_seq == n->timestamp_seq) {
			
			cfs_remove(n->file);
			PRINTF("CUSTODY REMOVE: file: %s \n", n->file);
			file_removed = n->length + CUSTODY_LIST_ENTRY_SIZE;
			list_remove(custody_list, n);
			memb_free(&custody_mem, n);
			save_custody_list();
			break;
		}
	}
	check_custody_list();
	custody_set_custody_etimer();
	return file_removed;
}
/** @} */

