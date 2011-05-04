#include "net/dtn/bundle.h"
#include "net/dtn/sdnv.h"
#include <stdlib.h>
#include <stdio.h>
#if CONTIKI_TARGET_SKY
	#include "net/dtn/realloc.h"
#endif
#include <string.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/** 
* brief creates a new bundle and allocates the minimum needed memory
*/
uint8_t create_bundle(struct bundle_t *bundle, uint8_t *payload, uint8_t len)
{
	if (len > 108){  //to be fragmented
		return 0;
	}
	bundle->offset_tab[VERSION][OFFSET]=0;
	bundle->offset_tab[FLAGS][OFFSET]=1;
	bundle->block = (uint8_t *) malloc(len+2);
	*bundle->block = 0;
	bundle->size=1;
	uint8_t i;
	bundle->offset_tab[VERSION][STATE]=1;
	for (i=1; i<=TYPE; i++){
		bundle->offset_tab[i][OFFSET]=1;
		bundle->offset_tab[i][STATE]=0;

	}
	bundle->offset_tab[TYPE][STATE]=1;
	memset(bundle->block+1,1,1);
	for (i=TYPE+1;i<20;i++){
		bundle->offset_tab[i][OFFSET]=2;
		bundle->offset_tab[i][STATE]=0;
	}
	bundle->size=len+2;
	memcpy(bundle->block + 2, payload, len);
	bundle->offset_tab[PAYLOAD][STATE] = len;
	uint8_t *tmp=bundle->block;
	/*for(i=0; i<bundle->size; i++){
		printf("%x ",*tmp);
		tmp++;
	}
	printf("\n"); 
	tmp = payload;
	for(i=0; i<len; i++){
		printf("%x ",*tmp);
		tmp++;
	}
	printf("\n");
	*/
	uint32_t len64=  len;
	set_attr(bundle, P_LENGTH, &len64);
	len64=0;
	set_attr(bundle, LENGTH, &len64);
	bundle->size=bundle->offset_tab[PAYLOAD][OFFSET]+bundle->offset_tab[PAYLOAD][STATE];
	return 1;
}


/**
*brief converts an integer value in sdnv and copies this to the right place in bundel
*/
uint8_t set_attr(struct bundle_t *bundle, uint8_t attr, uint32_t *val)
{
	if (attr == TYPE){
		uint8_t *tmp;
		tmp = bundle->block + bundle->offset_tab[TYPE][OFFSET];
		memset(tmp,(uint8_t) *val, 1);
		return 1;
	}
	sdnv_t sdnv;
	size_t len = sdnv_encoding_len(*val);
//	printf("tpr %u\n ",len);  // this fixes everything
	sdnv = (uint8_t *) malloc(len);
	sdnv_encode(*val,sdnv,len);
	if((len-bundle->offset_tab[attr][STATE]) > 0){
#if CONTIKI_TARGET_SKY
		bundle->block = (uint8_t *) realloc(bundle->block,(len-bundle->offset_tab[attr][STATE]) + bundle->size,bundle->size);
#else
		bundle->block = (uint8_t *) realloc(bundle->block,(len-bundle->offset_tab[attr][STATE]) + bundle->size);
#endif
		memmove((bundle->block + bundle->offset_tab[attr][OFFSET] + len), bundle->block + bundle->offset_tab[attr][OFFSET], bundle->size - bundle->offset_tab[attr][OFFSET] );
	}
	memcpy(bundle->block + bundle->offset_tab[attr][OFFSET], sdnv, len);
	uint8_t i;
	for(i=attr+1;i<20;i++){
		bundle->offset_tab[i][OFFSET] += (len - bundle->offset_tab[attr][STATE]);
	}
	bundle->size += (len - bundle->offset_tab[attr][STATE]);
	bundle->offset_tab[attr][STATE] = len;
	uint8_t size=0;
	if (attr >2 && attr <16){
		for (i=3; i<16; i++){
			size+=bundle->offset_tab[i][STATE];
		}
		memset(bundle->block+bundle->offset_tab[LENGTH][OFFSET],size,1);
	}
	free(sdnv);
	return 1;
}

uint8_t recover_bundel(struct bundle_t *bundle,uint8_t *block)
{
	PRINTF("rec bptr: %p  blptr:%p \n",bundle,block);
	bundle->offset_tab[VERSION][OFFSET]=0;
	bundle->offset_tab[VERSION][STATE]=1;
	bundle->offset_tab[FLAGS][OFFSET]=1;
	uint8_t *tmp=block;
	tmp+=1;
	uint8_t fields=0;
	if (*tmp & 0x40){ //fragmented	
		PRINTF("fragment\n");
		fields=15;
	}else{
		fields=13;
	}
	uint8_t i;
	for (i = 1; i<=fields; i++){
		uint8_t len= sdnv_len(tmp);
		bundle->offset_tab[i][STATE]=len;
		bundle->offset_tab[i][OFFSET]=tmp-block;
		tmp+=bundle->offset_tab[i][STATE];
	}
	if (!(*tmp & 0x40)){ //not fragmented
		bundle->offset_tab[FRAG_OFFSET][OFFSET] = bundle->offset_tab[LIFE_TIME][OFFSET]+1;
		bundle->offset_tab[APP_DATA_LEN][OFFSET] = bundle->offset_tab[LIFE_TIME][OFFSET]+1;
	}
	bundle->offset_tab[TYPE][OFFSET]=tmp-block;
	bundle->offset_tab[TYPE][STATE]=1;
	tmp+=1;
	bundle->offset_tab[P_FLAGS][STATE]=sdnv_len(tmp);
	bundle->offset_tab[P_FLAGS][OFFSET]=tmp-block;
	tmp+=bundle->offset_tab[P_FLAGS][STATE];
	bundle->offset_tab[P_LENGTH][STATE]=sdnv_len(tmp);
	bundle->offset_tab[P_LENGTH][OFFSET]=tmp-block;
	tmp+=bundle->offset_tab[P_LENGTH][STATE];
	uint32_t val;
	sdnv_decode(block+bundle->offset_tab[P_LENGTH][OFFSET],bundle->offset_tab[P_LENGTH][STATE],&val);
	bundle->offset_tab[PAYLOAD][STATE]= (uint8_t) val;
	bundle->offset_tab[PAYLOAD][OFFSET]= tmp-block;
	bundle->size=bundle->offset_tab[PAYLOAD][OFFSET]+bundle->offset_tab[PAYLOAD][STATE];
	bundle->block=block;
	return 1;
}
uint16_t delete_bundle(struct bundle_t *bundel)
{
	free(bundel->block);
	free(bundel);
}
