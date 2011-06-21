#include "bundle.h"
uint8_t b_stat_send(struct bundle_t *bundle,uint8_t status, uint8_t reason)
{
	struct mmem report;
	uint8_t size=3; //1 byte status + 1byte reason + 1byte timestamp (0)
	if( bundle->flags & 1){ //bundle fragment
		size += bundle->offset_tag[FRAG_OFFSET][STATE];
		uint8_t off=0;
		while( *((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + offset) != 0){
			uint16_t f_len, d_len;
			uint32_t len;
			f_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1);
			d_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len);
			sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len+d_len , dlen , &len);
			off+=f_len + d_len + len;
		}
		uint16_t f_len,d_len;
		uint32_t len;
		f_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1);
		d_len=sdnv_len((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len);
		sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET]+1+f_len+d_len , dlen , &len);
		size+=len;
	}

	size += bundle->offset_tab[TIME_STAMP][STATE];
	size += bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE];
	size += bundle->offset_tab[SRC_NODE][STATE];
	size += bundle->offset_tab[SRC_SERV][STATE];
	

	if (!mmem_alloc(report,size)){
		return 0;
	}
	*(uint8_t*) report.ptr = status;
	*(((uint8_t*) report.ptr)+1)= reason;
	*(((uint8_t*) report.ptr)+2)= 0;
	uint8_t offset=3;
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
	sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[REP_NODE][OFFSET],bundle->offset_tab[REP_NODE][STATE],&tmp);
	set_attr(&rep_bundle, DEST_NODE, &tmp);
	sdnv_decode((uint8_t*)bundle->mem.ptr + bundle->offset_tab[REP_SERV][OFFSET],bundle->offset_tab[REP_SERV][STATE],&tmp);
	set_attr(&rep_bundle, DEST_SERV, &tmp);

		
			


}

const struct status_report_driver b_stat ={
	"B_STAT",
	b_stat_send,
}
