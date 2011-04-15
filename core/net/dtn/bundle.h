struct prim_block_t{
	char offset_tab[16];
	uint8_t *block;	
	uint8_t size;
};

struct payload_block_t{
	char offset_tab[4];
	uint8_t *block;
	uint8_t size;
};

struct bundle_t{
	struct prim_block_t prim;	
	struct payload_block_t payload;
};
