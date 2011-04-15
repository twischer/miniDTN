struct prim_block_t{
	char offset_tab[16];
	uint8_t *block;	
};

struct payload_block_t{
	char offset_tab[4];
	uint8_t *block;
};

struct bundle_t{
	struct prim_block_t prim_block;	
	struct payload_block_t payload_block;
};
