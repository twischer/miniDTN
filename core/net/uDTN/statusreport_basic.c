/** 
* \addtogroup status
* @{
* \defgroup b_satus Basic status report modul
* @{
*/
#include <string.h>

#include "mmem.h"

#include "bundle.h"
#include "agent.h"
#include "storage.h"
#include "sdnv.h"

#include "statusreport.h"

static struct mmem report;
/**
* \brief sends a status report for a bundle to the "report-to"-node
* \param bundle pointer to bundle 
* \param status status code for the bundle
* \param reason reason code for the status
*/

/* XXX FIXME: Status reporting is not supported at the moment
uint8_t b_stat_send(struct bundle_t *bundle,uint8_t status, uint8_t reason)
{
	//printf("STAT: send %u %u\n",status,reason);
	uint8_t size=4; //1byte admin record +1 byte status + 1byte reason + 1byte timestamp (0)
	uint8_t type=16;
	uint16_t f_len,d_len;
	uint32_t len;
	uint8_t j;
	if( bundle->flags & 1){ //bundle fragment
		type +=1;
		size += bundle->offset_tab[FRAG_OFFSET][STATE];
		uint8_t off=0;
		while( *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + off) != 0){
			f_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+off);
			d_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len+off);
			sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len+d_len+off , d_len , &len);
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
		printf("STAT: mmem ERROR\n");
		return 0;
	}
	*((uint8_t*) MMEM_PTR(&report))= type;
	*((uint8_t*) MMEM_PTR(&report)+1)= status;
	*((uint8_t*) MMEM_PTR(&report)+2)= reason;
	uint8_t offset=3;
	printf("STAT: %p %p ",MMEM_PTR(&report),MMEM_PTR(&bundle->mem));
	for (j=0;j<bundle->mem.size;j++){
		printf("%x:",*((uint8_t*) bundle->mem.ptr+ j));
	}
	printf("\n");
	if( bundle->flags & 1){ 
		memcpy(((uint8_t*) report.ptr) + offset, bundle->mem.ptr + bundle->offset_tab[FRAG_OFFSET][OFFSET],bundle->offset_tab[TIME_STAMP][STATE]);
		offset+= bundle->offset_tab[FRAG_OFFSET][STATE];
		struct mmem sdnv;
		uint8_t sdnv_len= sdnv_encoding_len(len);
		if(!mmem_alloc(&sdnv,sdnv_len)){
			printf("STAT: mmem ERROR2\n");
			mmem_free(&report);
			return 0;
		}
		sdnv_encode(len, (uint8_t*) sdnv.ptr, sdnv_len);
		memcpy(((uint8_t*) report.ptr) + offset , sdnv.ptr , sdnv_len);
		offset+= sdnv_len;
		mmem_free(&sdnv);
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
	if(status&2){
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[CUST_NODE][OFFSET],bundle->offset_tab[CUST_NODE][STATE],&tmp);
	}else{
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[REP_NODE][OFFSET],bundle->offset_tab[REP_NODE][STATE],&tmp);
	}
	set_attr(&rep_bundle, DEST_NODE, &tmp);
	printf("STAT: %lu %u\n",tmp,status);
	if(status&2){
	
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[CUST_SERV][OFFSET],bundle->offset_tab[CUST_SERV][STATE],&tmp);
	}else{
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[REP_SERV][OFFSET],bundle->offset_tab[REP_SERV][STATE],&tmp);
	}
	set_attr(&rep_bundle, DEST_SERV, &tmp);
	printf("STAT: %lu \n",tmp);
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
	unsigned int old_size = rep_bundle.mem.size;
	if(!mmem_realloc(&rep_bundle.mem, rep_bundle.mem.size + size)){
		printf("STAT: mmem ERROR3\n");
		mmem_free(&report);
		mmem_free(&rep_bundle.mem);
		return 0;
	}
	memcpy((char *)rep_bundle.mem.ptr+ old_size, report.ptr, size);
	mmem_free(&report);
	uint8_t i;
	for (i=0; i< rep_bundle.mem.size; i++){
		printf("%x:",*((uint8_t *)rep_bundle.mem.ptr + i));
	}
	printf("\n");
	
	rep_bundle.size= rep_bundle.mem.size;
	int32_t saved=BUNDLE_STORAGE.save_bundle(&rep_bundle);
	printf("STAT: bundle_num %ld\n",saved);
	if (saved >=0){

		uint16_t *saved_as_num= memb_alloc(saved_as_mem);
		*saved_as_num= (uint16_t)saved;
		delete_bundle(&rep_bundle);
		process_post(&agent_process,dtn_bundle_in_storage_event, saved_as_num);
		return 1;
	}else{
		delete_bundle(&rep_bundle);
		return 0;
	}

}
*/

uint8_t b_stat_send(struct bundle_t *bundle,uint8_t status, uint8_t reason)
{
	return 0;
}

const struct status_report_driver statusreport_basic = {
	"B_STATUS",
	b_stat_send,
};
/** @} */
/** @} */
