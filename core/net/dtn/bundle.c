#include "net/dtn/bundle.h"
#include "net/dtn/sdnv.h"

/** 
* brief creates a new bundle and allocates the minimum needed memory
*/
uint8_t *create_bundle(uint8_t *payload, uint8_t len)
{
	struct bundle_t bundle;
	bundle.offset_tab[VERSION][OFFSET]=0;
	bundle.offset_tab[FLAGS][OFFSET]=1;
	bundle.block = (uint8_t *) malloc(len+2);
	*bundle.block = 0;
	bundle.size=1;
	uint8_t i;
	bundle.offset_tab[VERSION][STATE]=1;
	for (i=1; i<=TYPE; i++){
		bundle.offset_tab[i][OFFSET]=1;
		bundle.offset_tab[i][STATE]=1;

	}
	memset(bundle.block+1,1,1);
	for (i=TYPE+1;i<sizeof(bundle.offset_tab);i++){
		bundle.offset_tab[i]=2;
	}
	bundle.size=len+2;
	memcpy(bundle.block + 2, payload, len);
	bundle.offset_tab[PAYLOAD][STATE] = len;
	return &bundle;
}


/**
*brief converts an integer value in sdnv and copies this to the right place in bundel
*/
uint8_t set_attr(uint8_t *bundle, uint8_t attr, uint64_t *val)
{
	sdnv_t sdnv;
	size_t len = sdnv_encoding_len(*val);
	sdnv = (uint8_t *) malloc(len);
	sdnv_encode(*val,sdnv,len);
	bundle.block = (uint8_t *) realloc(bundle.block,(len-bundle.offset_tab[attr][STATE]) + bundle.size);
	memmove(bundle.block + bundle.offset_tab[attr][OFFSET] + len, bundle.block + bundle.offset_tab[attr][OFFSET], bundle.size - bundle.offset_tab[attr][OFFSET] );
	memcpy(bundle.block + bundle.offset_tab[attr][OFFSET], sdnv, len);
	uint8_t i;
	for(i=attr+1;i<sizeof(bundle.offset_tab);i++){
		bundle.offset_tab[i][OFFSET] += (len - bundle.offset_tab[attr][STATE]);
	}
	bundle.size += (len - bundle.offset_tab[attr][STATE]);
	bundle.offset_tab[attr][STATE] = len;
	free(sdnv);
}

uint8_t *recover_bundel(uint8_t *block)
{
	struct bundle_t bundle;
	bundle.offset_tab[VERSION]=0;
	bundle.offset_tab[FLAGS]=1;
	uint8_t *tmp=block;
	tmp+=1;
	if (*tmp & 0x40){ //fragmented	
	
	}else{

	}
}
