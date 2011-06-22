#include "net/dtn/custody-signal.h"
#include "net/dtn/custody.h"
#include "bundle.h"

struct cust_t {
	struct cust *next;
	uint16_t bundle_num;
	uint32_t src_node;
	uint32_t timestamp;
	uint32_t frag_offset;
	uint32_t seq_num;
}




LIST(cust_list);
MEMB(cust_mem, struct cust_t, MAX_CUST);

void b_cust_init(void)
{
	memb_init(&cust_mem);
	list_init(cust_list);
	return;
}


uint8_t b_cust_release(struct bundle_t *bundle)
{
	struct cust_t *cust;
	uint8_t offset=0;
	uint8_t frag = *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]) & 1;
	offset++;
	uint8_t status= *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
	offset++;
	uint32_t frag_offset, frag_len;
	uint8_t len;
	
	if (frag){
		len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &frag_offset);
		offset +=len;
		len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &frag_len);
		offset +=len;
	}

	uint32_t acc_time;
	len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
	sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &acc_time);
	offset +=len;

	uint32_t timestamp;
	len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
	sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &timestamp);
	offset +=len;

	uint32_t seq_num;
	len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
	sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &seq_num);
	offset +=len;

	uint32_t src_node;
	len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
	sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &src_node);
	offset +=len;

	


		

	// search custody 
	for(cust = list_head(cust_list); cust != NULL; cust= list_item_next(cust)){
		if( cust->src_node == src_node && cust->seq_num == seq_num && cust->timestamp == timestamp && cust->frag_offset == frag_offset);
			break;	
		}
	}
	// delete in storage
	BUNDLE_STORAGE.del_bundle(cust->bundle_num);
	// delete in list
	list_remove(cust_list,cust);
	// free memb
	memb_free(&cust_mem, cust);
	// delete_bundle
	delete_bundle(bundel);
	

	return 0;
}
uint8_t b_cust_deleted(struct bundle_t *bundle){
	//send deleted to reportto
	//delete in list 
	
}

int32_t b_cust_decide(struct bundle_t *bundle)
{
	if (BUNDLE_STORAGE.free_space(bundle) > 0){
		bundle->custody=1;
		int32_t saved= BUNDLE_STORAGE.save_bundle(bundle);
		if (saved>=0){
			struct cust_t *cust;
			cust = memb_alloc(&cust_mem);
			cust->frag_offset=0;
			cust->bundle_num= (uint16_t) saved;
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[SRC_NODE][OFFSET] , bundle->offset_tab[SRC_NODE][STATE] , &cust->src_node);
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[TIME_STAMP][OFFSET] , bundle->offset_tab[TIME_STAMP][STATE] , &cust->timestamp);
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET] , bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE] , &cust->src_node);
			if (bundle->flags & 1){
				sdnv_decode(bundle->mem.ptr + bundle->offset_tab[FRAG_OFFSET][OFFSET] , bundle->offset_tab[FRAG_OFFSET][STATE] , &cust->frag_offset);
			}
			list_add(cust_list, cust);

			
			//save saved to list
		return saved;
	}else{
		return -1;
	}
}


const struct custody_driver null_custody ={
	"B_CUSTODY",
	b_cust_init,
	b_cust_manage,
	b_cust_set_state,
	b_cust_decide,
};

