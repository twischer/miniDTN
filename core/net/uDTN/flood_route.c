/**
 * \addtogroup routing
 * @{
 */

 /**
 * \defgroup floodrouting Flooding Routing module
 *
 * @{
 */

/**
 * \file 
 * implementation of flooding
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
 */
#include "net/netstack.h"


#include "dtn_config.h"
#include "bundle.h"
#include "net/rime/rimeaddr.h"
#include "storage.h"
#include "sdnv.h"
#include "routing.h"
#include "agent.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "contiki.h"
#include <string.h>

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define ROUTING_MAX_MEM 10
#define ROUTING_NEI_MEM 1
/** struct to store the bundels to be routed */
struct pack_list_t {
	/** pointer to next bundle */
	struct pack_list_t *next;
	/** bundle number*/
	uint16_t num;
	/** bundle processing flags, destination node, source node, sequence number */
	uint32_t flags, dest_node, scr_node, seq_num;
	/** number of nodes this bundle was sent to */
	uint8_t send_to;
	/** 1 if bundle is in processing */
	uint8_t action;
	/** addresses of nodes this bundle was sent to */
	rimeaddr_t dest[ROUTING_NEI_MEM];
};

LIST(pack_list);
MEMB(pack_mem, struct pack_list_t, ROUTING_MAX_MEM);
LIST(route_list);

/**
* \brief called by agent at startup
*/
void flood_init(void)
{
	PRINTF("FLOOD: init flood_route\n");
	memb_init(&pack_mem);
	memb_init(&route_mem);
	list_init(pack_list);
	list_init(route_list);
	PRINTF("FLOOD: pack_list %p\n",list_head(pack_list));

	return;
}
/**
* \brief checks if there are bundle to send to dest
* \param dest pointer to the address of the new neighbor
*/
void flood_new_neigh(rimeaddr_t *dest)
{
	PRINTF("FLOOD: new node: %u:%u\n",dest->u8[1] ,dest->u8[0]);
	struct pack_list_t *pack;
	uint8_t count=0;
	for(pack = list_head(pack_list); pack != NULL; pack = list_item_next(pack)) {
		uint8_t sent=0,i;
		for (i =0 ; i < ROUTING_NEI_MEM ; i++) {
			PRINTF("FLOOD: bundle %u already sent to node %u:%u == %u:%u? %lu\n",pack->num, dest->u8[1] ,dest->u8[0], pack->dest[i].u8[1], pack->dest[i].u8[0], pack->scr_node);
			if ((pack->dest[i].u8[0] == dest->u8[0] && pack->dest[i].u8[1] == dest->u8[1])|| pack->scr_node == dest->u8[0] || pack->action==1){
				sent=1;
				if (pack->action==1){
				}
			}
		}
		if(!sent){
			struct route_t *route;
			route= memb_alloc(&route_mem);
			memcpy(route->dest.u8,dest->u8,sizeof(dest->u8));
			route->bundle_num=pack->num;
			pack->action=1;
			list_add(route_list,route);	
			count++;

		}
	}
	if (count){
		process_post(&agent_process,dtn_send_bundle_to_node_event, list_head(route_list));
	}
	return ;
}


void flood_delete_list(void)
{
	struct route_list_t *route;
	while(list_length(route_list)>0){
		PRINTF("FLOOD: list length %u\n",list_length(route_list));
		route=list_pop(route_list);
		memb_free(&route_mem,route);
	}
}

/**
* \brief addes a new bundle to the list of bundles
* \param bundle_num bundle number of the bundle
* \return >0 on success, <0 on error
*/
int flood_new_bundle(uint16_t bundle_num)
{
	struct pack_list_t *pack;
	for(pack = list_head(pack_list); pack != NULL; pack = list_item_next(pack)) {
		if (pack->num==bundle_num){
			PRINTF("FLOOD: bundle is already known\n");
			return -1;
		}
	}
	PRINTF("FLOOD: its new\n");
	pack =  memb_alloc(&pack_mem);
	if (pack !=NULL ){
		struct bundle_t bundle;
		pack->num=bundle_num;
		PRINTF("FLOOD: memory\n");
		if (BUNDLE_STORAGE.read_bundle(bundle_num, &bundle) <=0){
			PRINTF("\n\nread bundle ERROR\n\n");
			return -1;
		}
		PRINTF("FLOOD: red bundle\n");
		sdnv_decode(bundle.mem.ptr+bundle.offset_tab[FLAGS][OFFSET],bundle.offset_tab[FLAGS][STATE],&pack->flags);
		sdnv_decode(bundle.mem.ptr+bundle.offset_tab[DEST_NODE][OFFSET],bundle.offset_tab[DEST_NODE][STATE],&pack->dest_node);
		sdnv_decode(bundle.mem.ptr+bundle.offset_tab[SRC_NODE][OFFSET],bundle.offset_tab[SRC_NODE][STATE],&pack->scr_node);
		sdnv_decode(bundle.mem.ptr+bundle.offset_tab[TIME_STAMP_SEQ_NR][OFFSET],bundle.offset_tab[TIME_STAMP_SEQ_NR][STATE],&pack->seq_num);
		pack->send_to=0;
		uint8_t i;
		for (i=0; i<ROUTING_NEI_MEM; i++){
			pack->dest[i].u8[0]=0;
			pack->dest[i].u8[1]=0;
		}
		PRINTF("FLOOD: %u:%u\n",pack->dest[0].u8[0],pack->dest[0].u8[1]);
		list_add(pack_list,pack);
		PRINTF("FLOOD: pack_list %p\n",list_head(pack_list));
		delete_bundle(&bundle);
	}
	return 1;
}

/**
* \brief deletes bundle from list
* \param bundle_num bundle nuber of the bundle
*/
void flood_del_bundle(uint16_t bundle_num)
{
	PRINTF("FLOOD: delete bundle %u\n",bundle_num);
	struct pack_list_t *pack;
	for(pack = list_head(pack_list); pack != NULL; pack = list_item_next(pack)) {
		if( pack->num == bundle_num ){
			break;
		}
	}
	if (pack != NULL){
		PRINTF("FLOOD: deleting bundle %u\n",bundle_num);
		list_remove(pack_list ,pack);
		memb_free(&pack_mem,pack);
	}else{	
		PRINTF("FLOOD: bundle allready deleted\n");
	}
	return;
}

/**
* \brief callback function sets the status of a bundle in the list
* \param route pointer to route struct 
* \param status status code
* \num_tx number of retransmissions 
*/
void flood_sent(struct route_t *route,int status, int num_tx)
{
    struct pack_list_t *pack;
	pack = list_head(pack_list);
	PRINTF("FLOOD: pack_list %p, %u\n",list_head(pack_list),pack->num);
	for(pack = list_head(pack_list); pack != NULL && pack->num != route->bundle_num ; pack = list_item_next(pack)){
		PRINTF("FLOOD: pack_num:%u != %u\n",pack->num,route->bundle_num);
	}
	pack->action=0;
		
	switch(status) {
	  case MAC_TX_COLLISION:
		 //   printf("coll for %lu\n",pack->seq_num);
	    PRINTF("FLOOD: collision after %d tx\n", num_tx);
	    break;
	  case MAC_TX_NOACK:
		  //  printf("noack for %lu\n",pack->seq_num);
	    PRINTF("FLOOD: noack after %d tx\n", num_tx);
	    break;
	  case MAC_TX_OK:
	    PRINTF("FLOOD: sent after %d tx\n", num_tx);
	    if (pack !=NULL && pack->send_to < ROUTING_NEI_MEM){
		    memcpy(pack->dest[pack->send_to].u8,route->dest.u8,sizeof(route->dest.u8));
		    pack->send_to++;
		//    printf("ack for %lu\n",pack->seq_num);
		    PRINTF("FLOOD: bundle %u sent to %u nodes\n",route->bundle_num, pack->send_to);	
		    memb_free(&route_mem,route);
		    PRINTF("FLOOD: bundle %u cleared\n",route->bundle_num);

	    }else{
	    	    if(pack->send_to >= ROUTING_NEI_MEM){
		   	flood_del_bundle(route->bundle_num);
			uint16_t tmp= route->bundle_num;
			memb_free(&route_mem,route);
	    	    	PRINTF("FLOOD: bundle sent to max number of nodes, deleting bundle\n");
			BUNDLE_STORAGE.del_bundle(tmp,4);
		    }else{
		    	PRINTF("FLOOD: ERROR\n");
		    }
		    	
	    }
	    break;
	  default:
	    PRINTF("FLOOD: error %d after %d tx\n", status, num_tx);
	  }
}

const struct routing_driver flood_route ={
	"flood_route",
	flood_init,
	flood_new_neigh,
	flood_new_bundle,
	flood_del_bundle,
	flood_sent,
	flood_delete_list,
};

