#include "net/dtn/bundle.h"
#include "net/dtn/storage.h"
#include "net/dtn/sdnv.h"

struct nei_list_t {
	struct nei_list_t *next;
	rimeaddr_t *dest
};
struct pack_list_t {
	struct pack_list_t *next;
	uint16_t num;
	uint32_t flags, dest_node;
	struct nei_list_t *nei_l;
}
LIST(pack_list_t);
LIST(nei_list_t);
static struct nei_list_t nei_list;
static struct pack_list_t pack_list;

static struct route_t route;

void flood_init(void)
{
	return;
}

void flood_new_neigh(rimeaddr_t *dest)
{
	struct nei_list_t nei;
	memcpy(nei.dest,dest,sizeof(dest));
	list_add(nei_list,&nei);
	struct pack_list_t pack;
	for(pack = list_head(pack_list); pack != NULL; pack = list_item_next(pack)) {
		uint8_t sent=0;
		for (nei = list_head(pack.nei_l); nei !=NULL; nei = list_item_next(nei)) {
			if (nei.dest->u8[0] == dest->u8[0] && nei.dest->u8[1] == dest->u8[1]){
				sent=1;
			}
		}
		if(!sent){
			memcpy(route.dest,dest,sizeof(dest));
			route.bundle_num=pack.num;
			process_post(&agent_process,dtn_send_bundle_to_node_event, pack.num);

		}
	}
	return ;
}

void flood_new_bundle(uint16_t bundle_num)
{
	struct nei_list_t nei;
	struct pack_list_t pack;
	struct bundle_t bundle;
	pack.num=bundle_num;
	BUNDLE_STORAGE.read_bundle(bundle_num, &bundle);
	sdnv_decode(bundel.offset_tab[FLAGS][OFFSET],bundel.offset_tab[FLAGS][STATE],pack.flags);
	sdnv_decode(bundel.offset_tab[DEST_NODE][OFFSET],bundel.offset_tab[DEST_NODE][STATE],pack.dest_node);
	list_add(pack_list,&pack);
	delete_bundle(&bundle)
	return ;
}

void flood_del_bundle(uint16_t bundle_num)
{
	struct pack_list_t pack;
	for(pack = list_head(pack_list); pack != NULL; pack = list_item_next(pack)) {
		if( pack.num == bundle_num ){
			break;
		}
	}
	list_remove(pack_list ,pack);
	return;
}

void flood_sent(struct route_t *route,int status, int num_tx)
{
	
	switch(status) {
	  case MAC_TX_COLLISION:
	    PRINTF("FLOOD: collision after %d tx\n", num_tx);
	    break;
	  case MAC_TX_NOACK:
	    PRINTF("FLOOD: noack after %d tx\n", num_tx);
	    break;
	  case MAC_TX_OK:
	    PRINTF("FLOOD: sent after %d tx\n", num_tx);
	    struct nei_list_t nei;
	    struct pack_list_t pack;
	    for(pack = list_head(pack_list); pack != NULL && pack.num != route->bundle_num ; pack = list_item_next(pack)) ;
	    memcpy(nei.dest,route->dest,sizeof(dest));
	    list_add(pack.nei_l,&nei);
	    	
		
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

