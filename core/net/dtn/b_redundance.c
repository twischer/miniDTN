
static struct red_bundle_t *list;

uint8_t check(struct bundle_t *bundle)
{


}
uint8_t set(struct bundle_t *bundel)
{
	struct red_bundle_t *new_bundle= (struct *red_bundle_t) malloc sizeof(struct red_bundle_t);
	sdnv_decode(bundel->block + bundle->offset_tab[SRC_NODE][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[SRC_NODE][OFFSET]), &new_bundle->src);
	sdnv_decode(bundel->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET]), &new_bundle->seq_nr);
	sdnv_decode(bundel->block + bundle->offset_tab[FRAG_OFFSET][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[FRAG_OFFSET][OFFSET]), &new_bundle->fraq_offset);
	sdnv_decode(bundel->block + bundle->offset_tab[LIFE_TIME][OFFSET], sdnv_len(bundel->block + bundle->offset_tab[LIFE_TIME][OFFSET]), &new_bundle->lifetime);
//TODO struct füllen und einfügen	
}
