#include "bundle.h"
uint8_t b_stat_send(struct bundle_t *bundle,uint8_t status, uint8_t reason)
{
	struct mmem report;
	uint8_t size=3; //1 byte status + 1byte reason + 1byte timestamp (0)
	if( bundle->flags & 1){ //bundle fragment
		size += bundle->offset_tag[FRAG_OFFSET][STATE] + //data len len
	}

	char[100]=eid;


	*(uint8_t*) report.ptr = status:

}

const struct status_report_driver b_stat ={
	"B_STAT",
	b_stat_send,
}
