#include <stdlib.h>
#include <stdio.h>

#include "net/dtn/sdnv.h"
#include "net/dtn/redundance.h"
#include "net/dtn/b_redundance.h"
#include "net/dtn/bundle.h"

#include "contiki.h"
#include "sys/ctimer.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static struct red_bundle_t *list;
static uint8_t length=0;
static struct ctimer *b_red_timer;

uint8_t check(struct bundle_t *bundle)
{
	uint32_t src, seq_nr, frag_offset;
	struct red_bundle_t *tmp = list;
	sdnv_decode(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET]), &src);
	sdnv_decode(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET]), &seq_nr);
	sdnv_decode(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET]), &frag_offset);
	do{
		if( src == tmp->src &&
			seq_nr == tmp->seq_nr &&
			frag_offset == tmp->frag_offset){
				return 1;
		}
		tmp=tmp->next;
	}while(tmp->next != NULL);
	return 0;
}

uint8_t del(struct red_bundle_t *red_bundle)
{
	struct red_bundle_t *tmp =red_bundle->next;
	red_bundle->next = red_bundle->next->next;
	free(tmp);
}

uint8_t set(struct bundle_t *bundle)
{
	struct red_bundle_t *new_bundle;
	new_bundle = (struct red_bundle_t*) malloc(sizeof(struct red_bundle_t));
	new_bundle->next=NULL;
	sdnv_decode(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[SRC_NODE][OFFSET]), &new_bundle->src);
	sdnv_decode(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET]), &new_bundle->seq_nr);
	sdnv_decode(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[FRAG_OFFSET][OFFSET]), &new_bundle->frag_offset);
	sdnv_decode(bundle->block + bundle->offset_tab[LIFE_TIME][OFFSET], sdnv_len(bundle->block + bundle->offset_tab[LIFE_TIME][OFFSET]), &new_bundle->lifetime);
	
	if ( length+1 >= B_RED_MAX){
		PRINTF("B_REDUNDANCE: reduncance check list is full, deleting entry with shortest lifetime\n");
		struct red_bundle_t *oldest;
		struct red_bundle_t *tmp = list;
		uint32_t shortes_life=0xffffffff;
		do{
			if(tmp->next->lifetime <= shortes_life){
				shortes_life = tmp->next->lifetime;
				oldest=tmp;
			}
		}while( tmp->next != NULL);
		del(oldest);
	}else{
		length++;
	}

	new_bundle->next = list->next;
	list->next = new_bundle;
	return length;
}
void reduce_lifetime(void)
{
	struct red_bundle_t *oldest;
	struct red_bundle_t *tmp = list;
	PRINTF("B_REDUNDANCE: reducing lifetime\n");
	while( tmp->next != NULL && tmp !=NULL){
		tmp->next->lifetime-=5;
		PRINTF("B_REDUNDANCE: lifetime of bundle is %lu seconds\n",tmp->next->lifetime);
		if (tmp->next->lifetime <= 0){
			PRINTF("B_REDUNDANCE: deleting bundle form list\n");
			del(tmp);
			if (tmp->next == NULL){
				PRINTF("B_REDUNDANCE: last bundle\n");
				break;
			}
		}
		tmp=tmp->next;
	}
	ctimer_restart(b_red_timer);
		

}
void b_red_init(void)
{
	PRINTF("B_REDUNDANCE: starting\n");
	list= (struct red_bundle_t*) malloc(sizeof(struct red_bundle_t));
	list->next=NULL;
	b_red_timer = (struct ctimer*) malloc(sizeof(struct ctimer));
	ctimer_set(b_red_timer,CLOCK_SECOND*5,reduce_lifetime,NULL);
}

const struct redundance_check b_redundance ={
	"B_REDUNDANCE",
	b_red_init,
	check,
	set,
};
