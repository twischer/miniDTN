#include "net/dtn/bundle.h"
#include "net/dtn/sdnv.h"

uint8_t *create_bundle(void)
{
	struct bundle_t bundle;
	bundle.prim.offset_tab[0]=0;
	bundle.prim.offset_tab[1]=1;
	bundle.prim.block = (uint8_t *) malloc(1);
	*bundle.prim.block = 0;
	bundle.prim.size=1;

	bundle.payload.offset_tab[0]=0;
	bundle.payload.offset_tab[1]=1;
	bundle.payload.block = (uint8_t *) malloc(1);
	*bundle.payload.block = 1;
	bundle.payload.size=1;

	return &bundle;
}

uint8_t set_prim_attr(uint8_t *bundle, uint8_t attr, uint64_t *val)
{
	sdnv_t sdnv;
	size_t len = sdnv_encoding_len(*val);
	flags=(uint8_t *) malloc(len);
	sdnv_encode(*val,sdnv,len);
	bundle.prim.block=(uint8_t *)realloc(bundle.prim.block,len+bundle.prim.size);
	memcpy(bundle.prim.block+bundle.prim.offset_tab[attr],sdnv,len);
	free(sdnv);
}





