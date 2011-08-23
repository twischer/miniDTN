/**
 * \addtogroup bprocess
 */

/**
 * \file
 *
 */
 
 
#include <stdio.h>
#include <string.h>


#include "registration.h"
#include "agent.h"
#include "lib/memb.h"
#include "lib/list.h"

/* maximale Anzahl an registrierten Anwendungen */
#ifndef MAX_REGISTRATIONS
#define MAX_REGISTRATIONS		5
#endif

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif



LIST(registration_list);
MEMB(registration_mem, struct registration, MAX_REGISTRATIONS);


void registration_init(void) {
	
	memb_init(&registration_mem);
	list_init(registration_list);
	reg_list = registration_list;
	PRINTF("REGISTRATION: list pointer: %p\n",reg_list);
}

int registration_new_app(uint32_t app_id, struct process *application_process) {
	
	
	
	struct registration *n;
	PRINTF("REGISTRATION: list_head pointer: %p\n",list_head(registration_list));
	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		
		if (n->node_id == dtn_node_id && n->app_id == app_id){
			return 0;
		}
	}
	if(n == NULL) {
		
		n = memb_alloc(&registration_mem);
		if(n != NULL) {
			list_add(registration_list, n);
			n->node_id = dtn_node_id ;
			n->app_id = app_id ;
			n->status = APP_ACTIVE;
			n->application_process = application_process;
			return 1;
		}
		
	}
	return -1;
}

struct process *registration_get_process(uint32_t app_id)
{
	struct registration *n;
	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if(n->node_id == dtn_node_id && n->app_id == app_id) {
			return n->application_process;
		}
	}
	return NULL;
}

void registration_remove_app(uint32_t app_id) {
	
	
	struct registration *n;
	
	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		
		if(n->node_id == dtn_node_id && n->app_id == app_id) {
			
			list_remove(registration_list, n);
			memb_free(&registration_mem, n);
		}
	}
}


int registration_set_active(uint32_t app_id) {
	
	
	
	struct registration *n;
	
	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		
		if(n->node_id == dtn_node_id && n->app_id == app_id) {
			
			n->status = APP_ACTIVE;
			break;
		}
		
	}
	if(n == NULL)
		return -1;
	
	return n->status;
}

int registration_set_passive(uint32_t app_id) {
	
	
	struct registration *n;
	
	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		
		if(n->node_id == dtn_node_id && n->app_id == app_id) {
			
			n->status = APP_PASSIVE;
			break;
		}
	}
	
	if(n == NULL)
		return -1;
	
	return n->status;
}


int registration_return_status(uint32_t app_id) {
	
	
	struct registration *n;
	
	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		
		if(n->node_id == dtn_node_id && n->app_id == app_id) {
			
			return n->status;			
		}		
	}
	return -1;
}
/** @} */
