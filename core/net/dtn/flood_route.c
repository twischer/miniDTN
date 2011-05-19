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

void flood_init(void)
{
	return;
}

void flood_new_neigh(rimeaddr_t *dest)
{
	struct nei_list_t nei;
	memcpy(nei.dest,dest,sizeof(dest));
	list_add(nei_list,&nei);
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

void flood_sent(uint16_t bundle_num,uint8_t payload_len,rimeaddr_t dest)
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
	    for(pack = list_head(pack_list); pack != NULL && pack.num != bundle_num ; pack = list_item_next(pack)) ;
	    memcpy(nei.dest,&dest,sizeof(dest));
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

