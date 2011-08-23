/**
* \addtogroup custody 
* @{
*/
/**
* \defgroup b_cust basic custody module
* @{
*/
/**
* \file 
* implementation of a basic costody modul
* \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
*/

#include "net/dtn/custody-signal.h"
#include "dtn_config.h"
#include "net/dtn/custody.h"
#include "bundle.h"
#include "storage.h"
#include "memb.h"
#include "list.h"
#include "routing.h"
#include "sdnv.h"
#include "agent.h"
#include "status-report.h"
#include "mmem.h"

#define RETRANSMIT 1000
#define MAX_CUST 10 

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


struct cust_t {
	struct cust *next;
	uint16_t bundle_num;
	uint32_t src_node;
	uint32_t timestamp;
	uint32_t frag_offset;
	uint32_t seq_num;
	uint16_t retransmit_in;
};




LIST(cust_list);
MEMB(cust_mem, struct cust_t, MAX_CUST);

static struct ctimer b_cust_timer;

static uint8_t cust_cnt;
static uint16_t time;

void retransmit();
void b_cust_init(void)
{
	memb_init(&cust_mem);
	list_init(cust_list);
	time=5;
	cust_cnt=0;
	ctimer_set(&b_cust_timer,CLOCK_SECOND*time,&retransmit,NULL);
	return;
}

void retransmit(){
	//search bundle to be retranmited in cust_list
	struct cust_t *cust;
	uint16_t mintime=0xFFFF;
	for(cust = list_head(cust_list); cust != NULL; cust= list_item_next(cust)){
		//reduce all retransmit times
		if(cust->retransmit_in - time <=0){
			//retransmit bundle
			ROUTING.del_bundle(cust->bundle_num);
			if(!ROUTING.new_bundle(cust->bundle_num)){
				return;
			}
			//set retransmit time
			cust->retransmit_in = RETRANSMIT;

		}else{
			cust->retransmit_in -= time;
			if (cust->retransmit_in < mintime){
				mintime=cust->retransmit_in;
			}
		}
	}
	ctimer_set(&b_cust_timer,CLOCK_SECOND*mintime,retransmit,NULL);
	
}

uint8_t b_cust_release(struct bundle_t *bundle)
{
	struct cust_t *cust;
	uint8_t offset=0;
	uint8_t cust_sig=  *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]) & 32;
	uint8_t frag = *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]) & 1;
	offset++;
	uint8_t status= *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
	offset++;
	uint32_t frag_offset, frag_len;
	uint8_t len;
	if(!cust_sig){
		offset++;
	}
	if (frag){
		len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &frag_offset);
		offset +=len;
		len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &frag_len);
		offset +=len;
	}else{
		frag_offset=0;
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

	

	uint8_t inlist=0;
		

	// search custody 
	for(cust = list_head(cust_list); cust != NULL; cust= list_item_next(cust)){
//		PRINTF("B_CUST: searching...\n");
		printf("B_CUST: %lu==%lu %lu==%lu %lu==%lu %lu==%lu\n", cust->src_node, src_node ,cust->seq_num ,seq_num ,cust->timestamp ,timestamp ,cust->frag_offset ,frag_offset);
		if( cust->src_node == src_node && cust->seq_num == seq_num && cust->timestamp == timestamp && cust->frag_offset == frag_offset){
//			PRINTF("B_CUST: found bundle\n");
			inlist=1;
			break;	
		}
	}
	if (inlist){
		printf("B_CUST: release %u\n",cust->bundle_num);
		// delete in storage
		BUNDLE_STORAGE.del_bundle(cust->bundle_num,0xff);
		// delete in list
		list_remove(cust_list,cust);
		// free memb
		memb_free(&cust_mem, cust);
		
		if (cust_cnt>0){
			cust_cnt--;
		}
	}
	// delete_bundle
	PRINTF("B_CUST: delete bundle %p %p\n", bundle,bundle->mem);
	delete_bundle(bundle);
	

	return 0;
}

uint8_t b_cust_restransmit(struct bundle_t *bundle)
{
	PRINTF("B_CUST: retransmit\n");
	struct cust_t *cust;
	uint8_t offset=0;
	uint8_t frag = *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]) & 1;
	offset++;
	uint8_t status= *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
	offset++;
	uint32_t frag_offset, frag_len;
	uint8_t len;
	
	if (frag){
		PRINTF("B_CUST: fragment\n");
		len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &frag_offset);
		offset +=len;
		len = sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset);
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset, len, &frag_len);
		offset +=len;
	}else{
		frag_offset=0;
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

	

	uint8_t inlist=0;
		

	// search custody 
	for(cust = list_head(cust_list); cust != NULL; cust= list_item_next(cust)){
		if( cust->src_node == src_node && cust->seq_num == seq_num && cust->timestamp == timestamp && cust->frag_offset == frag_offset){
			inlist=1;
			break;	
		}
	}
	if( inlist){
		ROUTING.del_bundle(cust->bundle_num);
		if(!ROUTING.new_bundle(cust->bundle_num)){
			return 0;
		}
		PRINTF("B_CUST: bundle num %u\n",cust->bundle_num);
		cust->retransmit_in = RETRANSMIT;
	}
	// delete_bundle
	delete_bundle(bundle);
	

	return 0;
}


uint8_t b_cust_report(struct bundle_t *bundle, uint8_t status){
	PRINTF("B_CUST: send report\n");
	//send deleted to custodian
	struct mmem report;
	uint8_t size=3;
	uint8_t type=32;
	uint32_t len;
	if (bundle->flags &1){
		type +=1;
		size += bundle->offset_tab[FRAG_OFFSET][STATE];
		uint8_t off=0;
		uint8_t f_len,d_len;
		while( *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + off) != 0){
			f_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1);
			d_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len);
			sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len+d_len , d_len , &len);
			off+=f_len + d_len + len;
		}
		f_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+off);
		d_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len+off);
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len+d_len+off , d_len , &len);
		size+=len;
	}

	size += bundle->offset_tab[TIME_STAMP][STATE];
	size += bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE];
	size += bundle->offset_tab[SRC_NODE][STATE];
	size += bundle->offset_tab[SRC_SERV][STATE];
	if (!mmem_alloc(&report,size)){
		PRINTF("B_CUST: OOOPS\n");
		return 0;
	}
	*(uint8_t*) report.ptr = type;
	*(((uint8_t*) report.ptr)+1)= status;
	uint8_t offset=2;
	if( bundle->flags & 1){ 
		memcpy(((uint8_t*) report.ptr) + offset, bundle->mem.ptr + bundle->offset_tab[FRAG_OFFSET][OFFSET],bundle->offset_tab[TIME_STAMP][STATE]);
		offset+= bundle->offset_tab[FRAG_OFFSET][STATE];
		struct mmem sdnv;
		uint8_t sdnv_len= sdnv_encoding_len(len);
		if(mmem_alloc(&sdnv,sdnv_len)){
			sdnv_encode(len, (uint8_t*) sdnv.ptr, sdnv_len);
			memcpy(((uint8_t*) report.ptr) + offset , sdnv.ptr , sdnv_len);
			offset+= sdnv_len;
			mmem_free(&sdnv);
		}else{
			PRINTF("B_CUST: OOOPS1\n");
			return 0;
		}
	}
	*(((uint8_t*) report.ptr)+offset)= 0;
	offset+=1;
	memcpy(((uint8_t*) report.ptr) + offset, bundle->mem.ptr + bundle->offset_tab[TIME_STAMP][OFFSET],bundle->offset_tab[TIME_STAMP][STATE]);
	offset+= bundle->offset_tab[TIME_STAMP][STATE];
	memcpy(((uint8_t*) report.ptr) + offset, bundle->mem.ptr + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET], bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE]);
	offset+= bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE];
	memcpy(((uint8_t*) report.ptr) + offset, bundle->mem.ptr + bundle->offset_tab[SRC_NODE][OFFSET], bundle->offset_tab[SRC_NODE][STATE]);
	offset+= bundle->offset_tab[SRC_NODE][STATE];
	memcpy(((uint8_t*) report.ptr) + offset, bundle->mem.ptr + bundle->offset_tab[SRC_SERV][OFFSET], bundle->offset_tab[SRC_SERV][STATE]);

	struct bundle_t rep_bundle;
	create_bundle(&rep_bundle);
	uint32_t tmp;
	sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[CUST_NODE][OFFSET],bundle->offset_tab[CUST_NODE][STATE],&tmp);
	set_attr(&rep_bundle, DEST_NODE, &tmp);
	sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[CUST_SERV][OFFSET],bundle->offset_tab[CUST_SERV][STATE],&tmp);
	set_attr(&rep_bundle, DEST_SERV, &tmp);
	set_attr(&rep_bundle, SRC_NODE, &dtn_node_id);
	tmp=2;
	set_attr(&rep_bundle, FLAGS, &tmp);
	tmp=0;
	set_attr(&rep_bundle, SRC_SERV, &tmp);
	set_attr(&rep_bundle, REP_NODE, &tmp);
	set_attr(&rep_bundle, REP_SERV, &tmp);
	set_attr(&rep_bundle, CUST_NODE, &tmp);
	set_attr(&rep_bundle, CUST_SERV, &tmp);
	set_attr(&rep_bundle, TIME_STAMP, &tmp);
	set_attr(&rep_bundle,TIME_STAMP_SEQ_NR,&dtn_seq_nr);
	dtn_seq_nr++;
	tmp=3000;
	set_attr(&rep_bundle, LIFE_TIME, &tmp);
	struct mmem tmp_mem;
	
	if(mmem_alloc(&tmp_mem, rep_bundle.mem.size + report.size)){
		memcpy(tmp_mem.ptr , rep_bundle.mem.ptr , rep_bundle.mem.size);
		memcpy(tmp_mem.ptr+ rep_bundle.mem.size, report.ptr, report.size);
		mmem_free(&report);
		mmem_free(&rep_bundle.mem);
		memcpy(&rep_bundle.mem, &tmp_mem, sizeof(tmp_mem));
		mmem_reorg(&tmp_mem,&rep_bundle.mem);
	}else{
		PRINTF("B_CUST: OOOOPPS2\n");
		mmem_free(&report);
		mmem_free(&rep_bundle.mem);
	}
	//printf("B_CUST: len %u\n",
#if DEBUG
	uint8_t i;
	PRINTF("B_CUST: %u ::",rep_bundle.mem.size);
	for(i=0;i<rep_bundle.mem.size; i++){
		PRINTF("%x:",*((uint8_t *)rep_bundle.mem.ptr+i));
	}
	PRINTF("\n");
#endif
	rep_bundle.size = rep_bundle.mem.size;
	int32_t saved=BUNDLE_STORAGE.save_bundle(&rep_bundle);
	if (saved >=0){
		uint16_t *saved_as_num = memb_alloc(saved_as_mem);
		*saved_as_num = (uint16_t)saved;
		delete_bundle(&rep_bundle);
		process_post(&agent_process,dtn_bundle_in_storage_event, saved_as_num);
		printf("B_CUST: %u \n",*saved_as_num);
		return 1;
	}else{
		delete_bundle(&rep_bundle);
		return 0;
	}

	//delete in list 
	//if(!(status & 128)&&(status & 4))
	//	list_remove(cust_list,cust);
		// free memb
	//	memb_free(&cust_mem, cust);
	//}
}


int32_t b_cust_decide(struct bundle_t *bundle)
{
	PRINTF("B_CUST: decide %u\n",cust_cnt);
	uint8_t free=BUNDLE_STORAGE.free_space(bundle);
	uint32_t src, cust_node;
	sdnv_decode(bundle->mem.ptr + bundle->offset_tab[SRC_NODE][OFFSET],bundle->offset_tab[SRC_NODE][STATE],&src);
	
	sdnv_decode(bundle->mem.ptr + bundle->offset_tab[CUST_NODE][OFFSET],bundle->offset_tab[CUST_NODE][STATE],&cust_node);
	if (free > 0 && cust_cnt < MAX_CUST && (src!= dtn_node_id || cust_node == dtn_node_id)){
		bundle->custody=1;
		uint32_t tmp;
		sdnv_decode(bundle->mem.ptr + bundle->offset_tab[CUST_NODE][OFFSET],bundle->offset_tab[CUST_NODE][STATE],&tmp);
		set_attr(bundle,CUST_NODE,&dtn_node_id);
		int32_t saved= BUNDLE_STORAGE.save_bundle(bundle);
		if (saved>=0){
			printf("B_CUST: acc %u\n", ((uint16_t)saved));
			struct cust_t *cust;
			cust = memb_alloc(&cust_mem);
			cust->frag_offset=0;
			cust->bundle_num= (uint16_t) saved;
			PRINTF("B_CUST: cust->bundle_num= %u\n",cust->bundle_num);
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[SRC_NODE][OFFSET] , bundle->offset_tab[SRC_NODE][STATE] , &cust->src_node);
			PRINTF("B_CUST: cust->src_node= %lu\n",cust->src_node);
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[TIME_STAMP][OFFSET] , bundle->offset_tab[TIME_STAMP][STATE] , &cust->timestamp);
			PRINTF("B_CUST: cust->timestamp %lu\n",cust->timestamp);
			sdnv_decode(bundle->mem.ptr + bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET] , bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE] , &cust->seq_num);
			PRINTF("B_CUST: cust->seq_num %lu\n",cust->seq_num);
			struct cust_t *t_cust;
			for(t_cust = list_head(cust_list); t_cust != NULL; t_cust= list_item_next(t_cust)){
				if (cust->bundle_num == t_cust->bundle_num){
			//		printf("B_CUST: allready custodian\n");
					return saved;
				}
			}
			if (bundle->flags & 1){
				sdnv_decode(bundle->mem.ptr + bundle->offset_tab[FRAG_OFFSET][OFFSET] , bundle->offset_tab[FRAG_OFFSET][STATE] , &cust->frag_offset);
			}else{
				cust->frag_offset=0;
			}
			cust->retransmit_in=RETRANSMIT;
			//save saved to list
			list_add(cust_list, cust);
			cust_cnt++;
			if (cust->src_node != dtn_node_id){
				printf("B_CUST: %u != %u\n",cust->src_node,dtn_node_id);
				set_attr(bundle,CUST_NODE,&tmp);
				STATUS_REPORT.send(bundle, 2,0);
			}
		}
			
		return saved;
	}else{
		printf("B_CUST: cust_cnt > MAX_CUST %u %u\n",cust_cnt,free);
		return -1;
	}
}
void b_cust_del_from_list(uint16_t bundle_num)
{
	struct cust_t *t_cust;
	for(t_cust = list_head(cust_list); t_cust != NULL; t_cust= list_item_next(t_cust)){
		if (bundle_num == t_cust->bundle_num){
			list_remove(cust_list,t_cust);
			// free memb
			memb_free(&cust_mem, t_cust);
			if (cust_cnt>0){
				cust_cnt--;
			}
		}
	}
}

const struct custody_driver b_custody ={
	"B_CUSTODY",
	b_cust_init,
	b_cust_release,
	b_cust_report,
	b_cust_decide,
	b_cust_restransmit,
	b_cust_del_from_list,
};
/** @} */
/** @} */
