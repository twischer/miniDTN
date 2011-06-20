uint8_t b_stat_send(struct bundle_t *bundle,uint8_t status, uint8_t reason)
{
	
}

const struct status_report_driver b_stat ={
	"B_STAT",
	b_stat_send,
}
