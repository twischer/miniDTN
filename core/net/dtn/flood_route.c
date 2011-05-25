#include "net/netstack.h"

#include "dtn_config.h"
#include "net/dtn/bundle.h"
#include "net/rime/rimeaddr.h"
#include "net/dtn/storage.h"
#include "net/dtn/sdnv.h"
#include "routing.h"
#include "agent.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "contiki.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define ROUTING_MAX_MEM 10
#define ROUTING_ROUTE_MAX_MEM 10

struct pack_list_t {
	struct pack_list_t *next;
	uint16_t num;
	uint32_t flags, dest_node;
	uint8_t send_to;
	rimeaddr_t dest[ROUTING_NEI_MEM];
};

LIST(pack_list);
MEMB(pack_mem, struct pack_list_t, ROUTING_MAX_MEM);
MEMB(route_mem, struct route_t, ROUTING_ROUTE_MAX_MEM);


void flood_init(void)
{
	PRINTF("FLOOD: init flood_route\n");
	memb_init(&pack_mem);
	memb_init(&route_mem);
	list_init(pack_list);
	PRINTF("FLOOD: pack_list %p\n",list_head(pack_list));

	return;
}

void flood_new_neigh(rimeaddr_t *dest)
{
	PRINTF("FLOOD: new node: %u:%u\n",dest->u8[1] ,dest->u8[0]);
	struct pack_list_t *pack;
	PRINTF("FLOOD: pack_list %p\n",list_head(pack_list));
	for(pack = list_head(pack_list); pack != NULL; pack = list_item_next(pack)) {
		PRINTF("FLOOD: searching for bundles\n");
		uint8_t sent=0,i;
		for (i =0 ; i < ROUTING_NEI_MEM ; i++) {
			PRINTF("FLOOD: bundle already sent to node %u:%u == %u:%u?\n", dest->u8[1] ,dest->u8[0], pack->dest[i].u8[1], pack->dest[i].u8[1]);
			if (pack->dest[i].u8[0] == dest->u8[0] && pack->dest[i].u8[1] == dest->u8[1]){
				PRINTF("FLOOD: YES\n");
				sent=1;
			}
		}
		if(!sent){
			struct route_t *route;
//			route= (struct route_t *)malloc(sizeof(dest)+sizeof(pack->num));
			route= memb_alloc(&route_mem);
			memcpy(route->dest.u8,dest->u8,sizeof(dest->u8));
			route->bundle_num=pack->num;
			PRINTF("FLOOD: send bundle %u to %u:%u \n",route->bundle_num, route->dest.u8[1] ,route->dest.u8[0]);

			process_post(&agent_process,dtn_send_bundle_to_node_event, route);

		}
	}
	return ;
}

void flood_new_bundle(uint16_t bundle_num)
{
	PRINTF("FLOOD: got new bundle %u\n",bundle_num);
	struct pack_list_t *pack;
	pack =  memb_alloc(&pack_mem);
	if (pack !=NULL ){
		struct bundle_t bundle;
		pack->num=bundle_num;
		BUNDLE_STORAGE.read_bundle(bundle_num, &bundle);
		sdnv_decode(bundle.offset_tab[FLAGS][OFFSET],bundle.offset_tab[FLAGS][STATE],&pack->flags);
		sdnv_decode(bundle.offset_tab[DEST_NODE][OFFSET],bundle.offset_tab[DEST_NODE][STATE],&pack->dest_node);
		pack->send_to=0;
		list_add(pack_list,pack);
		PRINTF("FLOOD: pack_list %p\n",list_head(pack_list));
		PRINTF("FLOOD: bundle saved to list\n");

		delete_bundle(&bundle);
	}
	return ;
}

void flood_del_bundle(uint16_t bundle_num)
{
	struct pack_list_t *pack;
	for(pack = list_head(pack_list); pack != NULL; pack = list_item_next(pack)) {
		if( pack->num == bundle_num ){
			break;
		}
	}
	if (pack != NULL){
		list_remove(pack_list ,pack);
		memb_free(&pack_mem,pack);
	}
	return;
}

void flood_sent(struct route_t *route,int status, int num_tx)
{
    struct pack_list_t *pack;
		
	switch(status) {
	  case MAC_TX_COLLISION:
	    PRINTF("FLOOD: collision after %d tx\n", num_tx);
	    break;
	  case MAC_TX_NOACK:
	    PRINTF("FLOOD: noack after %d tx\n", num_tx);
	    break;
	  case MAC_TX_OK:
	    PRINTF("FLOOD: sent after %d tx\n", num_tx);
	    PRINTF("FLOOD: pack_list %p\n",list_head(pack_list));
	    for(pack = list_head(pack_list); pack != NULL && pack->num != route->bundle_num ; pack = list_item_next(pack)) ;
	    if (pack !=NULL && pack->send_to < ROUTING_NEI_MEM){
		    memcpy(pack->dest[pack->send_to].u8,route->dest.u8,sizeof(route->dest.u8));
		    pack->send_to++;
		    PRINTF("FLOOD: bundle %u sent to %u nodes\n",route->bundle_num, pack->send_to);	
		    memb_free(&route_mem,route);
	    }else{
	    	    if(pack->send_to >= ROUTING_NEI_MEM){
		   	flood_del_bundle(route->bundle_num);
			uint16_t tmp= route->bundle_num;
			memb_free(&route_mem,route);
	    	    	PRINTF("FLOOD: bundle sent to max number of nodes, deleting bundle\n");
			BUNDLE_STORAGE.delete_bundle(tmp);
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
};

