void null_cust_init(void)
{
	return;
}

uint8_t null_cust_manage(struct bundel_t *bundle)
{
	//TODO admin record senden, dass bundle abgelehnt wurde
	return 0;
}

uint8_t null_cust_set_state(uint8_t state, uint8_t reason, struct bundel_t *bundle)
{
	return 0;
}

uint8_t null_cust_decide(struct bundle_t *bundle)
{
	return 0;
}


const struct custody_driver null_custody ={
	"NULL_CUSTODY",
	null_cust_init,
	null_cust_manage,
	null_cust_set_state,
	null_cust_decide,
};

