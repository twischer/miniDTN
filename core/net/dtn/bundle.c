#include "net/dtn/bundle.h"
#include "net/dtn/sdnv.h"

/** 
* brief creates a new bundle and allocates the minimum needed memory
*/
uint8_t *create_bundle(void)
{
	struct bundle_t bundle;
	bundle.offset_tab[VERSION]=0;
	bundle.offset_tab[FLAGS]=1;
	bundle.block = (uint8_t *) malloc(1);
	*bundle.block = 0;
	bundle.size=1;
	uint8_t i;
	for (i=1; i<sizeof(bundle.offset_tab); i++){
		bundle.offset_tab[i]=1
	}
	set_type(i);
	return &bundle;
}

void set_type(uint8_t val)
{
	memmove(bundle.block+bundle.offset_tab[TYPE]+1,bundle.block+bundle.offset_tab[TYPE],bundle.size-bundle.offset_tab[TYPE]);
	memset(bundle.block+bundle.offset_tab[TYPE],1,1);
	uint8_t i;
	for(i=TYPE+1;i<sizeof(bundle.offset_tab);i++){
		bundle.offset_tab[i]+=1;
	}
	bundle.size+=1;
}

/**
*brief converts an integer value in sdnv and copies this to the right place in bundel
*/
uint8_t set_attr(uint8_t *bundle, uint8_t attr, uint64_t *val)
{
	sdnv_t sdnv;
	size_t len = sdnv_encoding_len(*val);
	sdnv=(uint8_t *) malloc(len);
	sdnv_encode(*val,sdnv,len);
	bundle.block=(uint8_t *)realloc(bundle.block,len+bundle.size);
	memmove(bundle.block+bundle.offset_tab[attr]+len,bundle.block+bundle.offset_tab[attr],bundle.size-bundle.offset_tab[attr]);
	memcpy(bundle.block+bundle.offset_tab[attr],sdnv,len);
	uint8_t i;
	for(i=attr+1;i<sizeof(bundle.offset_tab);i++){
		bundle.offset_tab[i]+=len;
	}
	bundle.size+=len;
	free(sdnv);
}

