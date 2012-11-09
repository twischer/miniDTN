/**
 * \addtogroup bprocess
 */

/**
 * \file
 *
 */
 
 
#include <stdio.h>
#include <string.h>

#include "lib/memb.h"
#include "lib/list.h"
#include "logging.h"

#include "api.h"
#include "agent.h"

#include "registration.h"

/* Maximum number of registered services */
#ifdef CONF_MAX_REGISTRATIONS
#define MAX_REGISTRATIONS CONF_MAX_REGISTRATIONS
#else
#define MAX_REGISTRATIONS		5
#endif

LIST(registration_list);
MEMB(registration_mem, struct registration, MAX_REGISTRATIONS);

void registration_init(void)
{
	/* Initialize the lists */
	memb_init(&registration_mem);
	list_init(registration_list);

	/* Set the pointer for "outsiders" */
	reg_list = registration_list;

	LOG(LOGD_DTN, LOG_AGENT, LOGL_INF, "Registration init ok");
}

int registration_new_application(uint32_t app_id, struct process *application_process, uint32_t node_id)
{
	struct registration * n = NULL;

	if( node_id == 0 ) {
		node_id = dtn_node_id;
	}

	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Registering app_id %lu on node_id %lu", app_id, node_id);

	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if (n->node_id == node_id && n->app_id == app_id){
			return 0;
		}
	}

	if( n != NULL ) {
		return -1;
	}

	n = memb_alloc(&registration_mem);
	if( n == NULL ) {
		return -1;
	}

	n->node_id = node_id;
	n->app_id = app_id;
	n->status = APP_ACTIVE;
	n->application_process = application_process;

	list_add(registration_list, n);

	return 1;
}

struct process * registration_get_process(uint32_t app_id, uint32_t node_id)
{
	struct registration * n = NULL;

	if( node_id == 0 ) {
		node_id = dtn_node_id;
	}

	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if(n->node_id == node_id && n->app_id == app_id) {
			return n->application_process;
		}
	}

	return NULL;
}

void registration_remove_application(uint32_t app_id, uint32_t node_id)
{
	struct registration * n = NULL;
	
	if( node_id == 0 ) {
		node_id = dtn_node_id;
	}

	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if(n->node_id == node_id && n->app_id == app_id) {
			list_remove(registration_list, n);
			memb_free(&registration_mem, n);
		}
	}
}

int registration_set_active(uint32_t app_id, uint32_t node_id) {
	struct registration * n = NULL;
	
	if( node_id == 0 ) {
		node_id = dtn_node_id;
	}

	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if(n->node_id == node_id && n->app_id == app_id) {
			n->status = APP_ACTIVE;
			break;
		}
	}

	if(n == NULL) {
		return -1;
	}
	
	return n->status;
}

int registration_set_passive(uint32_t app_id, uint32_t node_id)
{
	struct registration * n = NULL;
	
	if( node_id == 0 ) {
		node_id = dtn_node_id;
	}

	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if(n->node_id == node_id && n->app_id == app_id) {
			n->status = APP_PASSIVE;
			break;
		}
	}
	
	if(n == NULL) {
		return -1;
	}
	
	return n->status;
}

int registration_return_status(uint32_t app_id, uint32_t node_id)
{
	struct registration * n = NULL;
	
	if( node_id == 0 ) {
		node_id = dtn_node_id;
	}

	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if(n->node_id == node_id && n->app_id == app_id) {
			return n->status;			
		}		
	}

	return -1;
}

uint32_t registration_get_application_id(struct process * application_process)
{
	struct registration * n = NULL;

	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if( n->application_process == application_process ) {
			return n->app_id;
		}
	}

	return 0xFFFF;
}

uint8_t registration_is_local(uint32_t app_id, uint32_t node_id)
{
	struct registration * n = NULL;

	if( node_id == 0 ) {
		node_id = dtn_node_id;
	}

	for(n = list_head(registration_list); n != NULL; n = list_item_next(n)) {
		if(n->node_id == node_id && n->app_id == app_id) {
			return 1;
		}
	}

	return 0;
}

/** @} */
