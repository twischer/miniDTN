#include <stdlib.h>
#include <stdio.h>

#include "net/dtn/sdnv.h"
#include "net/dtn/redundance.h"
#include "net/dtn/b_redundance.h"
#include "net/dtn/bundle.h"
#include "lib/list.h"
#include "lib/memb.h"



#include "contiki.h"
#include "sys/ctimer.h"



#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
static uint16_t CUSTODY_LIST_ENTRY_SIZE;

LIST(b_red_list);
MEMB(b_red_mem, struct red_bundle_t, B_RED_MAX);







static uint8_t length=0;
static struct ctimer *b_red_timer;

uint8_t check(struct bundle_t *bundle)
{
	PRINTF("REDUNDANCE: check\n");	
	uint32_t src, seq_nr, frag_offset;
	sdnv_decode(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET]), &src);
	sdnv_decode(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET]), &seq_nr);
	sdnv_decode(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET]), &frag_offset);
	struct red_bundle_t *n;
	for(n = list_head(b_red_list); n != NULL; n = list_item_next(n)) {
		if( src == n->src &&
			seq_nr == n->seq_nr &&
			frag_offset == n->frag_offset){
				PRINTF("REDUNDANCE: redundant\n");	
				return 1;
		}
		PRINTF("REDUNDANCE: not redundant: %lu != %lu : %lu != %lu : %lu != %lu\n",src,n->src,seq_nr,n->seq_nr,frag_offset,n->frag_offset);	
	}
	return 0;
}


uint8_t set(struct bundle_t *bundle)
{

	struct red_bundle_t *n;
	uint32_t src,seq_nr,frag_offset,lifetime;
	sdnv_decode(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET]), &src);
	sdnv_decode(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET]), &seq_nr);
	sdnv_decode(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET]), &frag_offset);
	sdnv_decode(bundle->block + bundle->offset_tab[LIFE_TIME][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[LIFE_TIME][OFFSET]), &lifetime);
	for(n = list_head(b_red_list); n != NULL; n = list_item_next(n)) {
		if( src == n->src &&
			seq_nr == n->seq_nr &&
			frag_offset == n->frag_offset){
				PRINTF("REDUNDANCE: redundant\n");	
				return 1;
		}
	}
	if(n == NULL) {
		n = memb_alloc(&b_red_mem);
		if(n != NULL) {
			n->seq_nr=seq_nr;
			n->src=src;
			n->frag_offset=frag_offset;
			n->lifetime=lifetime;
			list_add(b_red_list, n);
		}else{
			return 0;
		}
	}
	return 1;
}

		

void reduce_lifetime(void)
{
//	PRINTF("B_REDUNDANCE: reducing lifetime\n");
	struct red_bundle_t *tmp;
	struct red_bundle_t *n;


	uint32_t i=0;
	n=list_head(b_red_list);
	for(tmp = list_head(b_red_list); tmp != NULL; tmp = list_item_next(tmp)) {
		PRINTF("B_REDUNDANCE: lifetime of bundle %lu is %lu seconds\n",i,tmp->lifetime);
		i++;
		if (tmp->lifetime <= 5){
			PRINTF("B_REDUNDANCE: deleting bundle form list\n");
			list_remove(b_red_list,tmp);
			memb_free(&b_red_mem, tmp);
		}else{
			tmp->lifetime-=5;
		}
		n=tmp;
	}
	ctimer_restart(b_red_timer);
		

}
void b_red_init(void)
{
	PRINTF("B_REDUNDANCE: starting\n");
	memb_init(&b_red_mem);
	list_init(b_red_list);
	CUSTODY_LIST_ENTRY_SIZE = sizeof(struct red_bundle_t) - sizeof(struct red_bundle_t *);
	b_red_timer = (struct ctimer*) malloc(sizeof(struct ctimer));
	ctimer_set(b_red_timer,CLOCK_SECOND*5,reduce_lifetime,NULL);
}

const struct redundance_check b_redundance ={
	"B_REDUNDANCE",
	b_red_init,
	check,
	set,
};
