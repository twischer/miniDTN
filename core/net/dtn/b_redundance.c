
static struct red_bundle_t *list;
static uint8_t length=0;

uint8_t check(struct bundle_t *bundle)
{
	uint32_t src, seq_nr, frag_offset;
	struct red_bundle_t *tmp = list;
	sdnv_decode(bundel->block + bundle->offset_tab[SRC_NODE][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[SRC_NODE][OFFSET]), &src);
	sdnv_decode(bundel->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET]), &seq_nr);
	sdnv_decode(bundel->block + bundle->offset_tab[FRAG_OFFSET][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[FRAG_OFFSET][OFFSET]), &frag_offset);
	do{
		if( src == tmp->src &&
			seq_nr == tmp->seq_nr &&
			frag_offset == tmp->frag_offset){
				return 1;
		}
		tmp=tmp->next;
	}while(tmp->next != NULL)
	return 0;
}

uint8_t del(struct red_bundle_t *red_bundle)
{
	struct red_bundle_t *tmp =red_bundle->next;
	red_bundle->next = red_bundle->next->next;
	free(tmp);
}

uint8_t set(struct bundle_t *bundel)
{
	struct red_bundle_t *new_bundle= (struct *red_bundle_t) malloc sizeof(struct red_bundle_t);
	sdnv_decode(bundel->block + bundle->offset_tab[SRC_NODE][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[SRC_NODE][OFFSET]), &new_bundle->src);
	sdnv_decode(bundel->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET]), &new_bundle->seq_nr);
	sdnv_decode(bundel->block + bundle->offset_tab[FRAG_OFFSET][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[FRAG_OFFSET][OFFSET]), &new_bundle->frag_offset);
	sdnv_decode(bundel->block + bundle->offset_tab[LIFE_TIME][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[LIFE_TIME][OFFSET]), &new_bundle->lifetime);
	
	if (lenght+1 >= B_RED_MAX){
		struct red_bundle_t *oldest;
		struct red_bundle_t *tmp = list;
		uint32_t shortes_life=0xffffffff;
		do{
			if(tmp->next->liftime <= shortes_life){
				shortes_life = tmp->next->liftime;
				oldest=tmp;
			}
		}while( tmp->next != NULL)
		del(oldest);
	}else{
		length++;
	}

	new_bundle->next = list->next;
	list->next = new_bundle;
	return length;
}
