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
	struct nei_list_t *;
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
	return ;
}
void flood_del_bundle(uint16_t bundle_num)
{
	return;
}

const struct routing_driver flood_route ={
	"flood_route",
	flood_init,
	flood_new_neigh,
	flood_new_bundle,
	flood_del_bundle,
};

