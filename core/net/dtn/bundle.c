#include "net/dtn/bundle.h"
#include "net/dtn/sdnv.h"
#include "mmem.h"
#include <stdlib.h>
#include <stdio.h>
#if CONTIKI_TARGET_SKY
//	#include "net/dtn/realloc.h"
#endif
#include <string.h>
#include "clock.h"

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
uint8_t create_bundle(struct bundle_t *bundle)
{
	bundle->offset_tab[VERSION][OFFSET]=0;
	bundle->offset_tab[FLAGS][OFFSET]=1;
	bundle->mem = (struct mmem*) malloc(sizeof(struct mmem));
	mmem_alloc(bundle->mem,1);
	bundle->block = (uint8_t *) MMEM_PTR(bundle->mem);
	if (bundle->block==NULL){
		PRINTF("\n\n MALLOC ERROR\n\n");
	}

	*bundle->block = 0;
	bundle->size=1;
	uint8_t i;
	bundle->rec_time=(uint32_t) clock_seconds(); 
	bundle->offset_tab[VERSION][STATE]=1;
	for (i=1;i<18;i++){
		bundle->offset_tab[i][OFFSET]=1;
		bundle->offset_tab[i][STATE]=0;
	}
	//memcpy(bundle->block + 2, payload, len);
	//bundle->offset_tab[PAYLOAD][STATE] = len;
	//uint8_t *tmp=bundle->block;
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
	uint32_t len64 ;
	//set_attr(bundle, P_LENGTH, &len64);
	len64=0;
	i=set_attr(bundle, LENGTH, &len64);
	i+=set_attr(bundle, DIRECTORY_LEN, &len64);
	bundle->size += len64 + i; 
#if DEBUG
	PRINTF("BUNDLE: CREATE ");
	uint8_t* tmp= bundle->block;
	for (i=1;i<bundle->offset_tab[DATA][OFFSET];i++){
		PRINTF("%u ",*tmp);
		tmp++;
	}
	PRINTF("\n");
#endif
	return 1;
}

uint8_t add_block(struct bundle_t *bundle, uint8_t type, uint8_t flags, uint8_t *data, uint8_t d_len)
{
	sdnv_t s_len;
	size_t len = sdnv_encoding_len((uint32_t )d_len);
	s_len = (uint8_t *) malloc(len);
	if (s_len==NULL){
		PRINTF("\n\n MALLOC ERROR\n\n");
	}

	sdnv_encode((uint32_t) d_len, s_len, len);
	
	struct mmem *mmem_tmp = (struct mmem*) malloc(sizeof(struct mmem));
	mmem_alloc(mmem_tmp,d_len + len + 2  + bundle->size);
	memcpy(mmem_tmp->ptr,bundle->block,bundle->size);
	mmem_free(bundle->mem);
	free(bundle->mem),
	bundle->mem=mmem_tmp;

/*#if CONTIKI_TARGET_SKY
	bundle->block = (uint8_t *) realloc(bundle->block,d_len + len + 2  + bundle->size,bundle->size);
#else
	bundle->block = (uint8_t *) realloc(bundle->block,d_len + len + 2  + bundle->size);
#endif
*/
	if (bundle->block == NULL) {
		return 0;
	}
	memcpy(bundle->block + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE], &type, 1);
	bundle->offset_tab[DATA][STATE] +=1;
	memcpy(bundle->block + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE], &flags, 1);
	bundle->offset_tab[DATA][STATE] +=1;
	memcpy(bundle->block + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE], s_len, len);
	bundle->offset_tab[DATA][STATE] +=len;
	memcpy(bundle->block + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE],data,d_len);
	bundle->offset_tab[DATA][STATE] +=d_len;
	bundle->size = bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE]; 
#if DEBUG
	uint8_t *tmp= bundle->block + bundle->offset_tab[DATA][OFFSET];
	PRINTF("BUNDLE: ADD_BLOCK: Type: %u , ",*tmp);
	tmp++;
	PRINTF("flags: %u , ",*tmp);
	tmp++;
	PRINTF("data_len: %u, data: ",d_len);
	tmp++;
	uint8_t i;
	for (i=0; i<d_len; i++){
		PRINTF("%u ",*tmp);
		tmp++;
	}
	PRINTF("\n");
#endif
	return d_len + len + 2;
}
/**
*brief converts an integer value in sdnv and copies this to the right place in bundel
*/
uint8_t set_attr(struct bundle_t *bundle, uint8_t attr, uint32_t *val)
{
	if (attr == FLAGS){
		bundle->custody = 0x08 &(uint8_t) *val;
	}
	if( attr == LIFE_TIME){
		bundle->lifetime= *val;
	}
	sdnv_t sdnv;
	size_t len = sdnv_encoding_len(*val);
//	printf("tpr %u\n ",len);  // this fixes everything
	sdnv = (uint8_t *) malloc(len);
	if (sdnv==NULL){
		PRINTF("\n\n MALLOC ERROR\n\n");
	}
				
	sdnv_encode(*val,sdnv,len);
	if(((int16_t)(len-bundle->offset_tab[attr][STATE])) > 0){
		PRINTF("BUNDLE: realloc %u\n",(len-bundle->offset_tab[attr][STATE]) + bundle->size);
	
		struct mmem *mmem_tmp = (struct mmem*) malloc(sizeof(struct mmem));
		mmem_alloc(mmem_tmp,(len-bundle->offset_tab[attr][STATE]) + bundle->size);
		memcpy(mmem_tmp->ptr,bundle->block,bundle->offset_tab[attr][OFFSET]);
		memcpy(mmem_tmp->ptr + bundle->offset_tab[attr][OFFSET]+(len-bundle->offset_tab[attr][STATE]), bundle->block + bundle->offset_tab[attr][OFFSET], bundle->size - bundle->offset_tab[attr][OFFSET]);
		mmem_free(bundle->mem);
		free(bundle->mem),
		bundle->mem=mmem_tmp;
		/*
#if CONTIKI_TARGET_SKY
		bundle->block = (uint8_t *) realloc(bundle->block,((int16_t)(len-bundle->offset_tab[attr][STATE])) + bundle->size,bundle->size);
		PRINTF("BUNDLE: mem-size %u\n",((int16_t)(len-bundle->offset_tab[attr][STATE])) + bundle->size);
#else
		bundle->block = (uint8_t *) realloc(bundle->block,((int16_t)(len-bundle->offset_tab[attr][STATE])) + bundle->size);
#endif
*/
	//	memmove((bundle->block + bundle->offset_tab[attr][OFFSET] + len), bundle->block + bundle->offset_tab[attr][OFFSET], bundle->size - bundle->offset_tab[attr][OFFSET] );
	}
	if (((int16_t)(len-bundle->offset_tab[attr][STATE])) < 0){
		PRINTF("BUNDLE: smaller\n");
		struct mmem *mmem_tmp = (struct mmem*) malloc(sizeof(struct mmem));
		mmem_alloc(mmem_tmp,bundle->size + ((int16_t)(len-bundle->offset_tab[attr][STATE])));
		uint8_t *tmp=(uint8_t*) MMEM_PTR(mmem_tmp);
		if (*tmp==NULL){
			PRINTF("\n\n MALLOC ERROR\n\n");
		}

		memcpy(tmp,bundle->block,bundle->offset_tab[attr][OFFSET]);
		memcpy(tmp + bundle->offset_tab[attr][OFFSET] + len,bundle->block + bundle->offset_tab[attr][OFFSET] + bundle->offset_tab[attr][STATE],bundle->size + ((int16_t)(len-bundle->offset_tab[attr][STATE])) );
		mmem_free(bundle->mem);
		free(bundle->mem);
		bundle->mem=mmem_tmp;

		bundle->block=tmp;
	}
	memcpy(bundle->block + bundle->offset_tab[attr][OFFSET], sdnv, len);
	uint8_t i;
	PRINTF("BUNDLE: val= %lu\n",*val);
	for(i=attr+1;i<18;i++){
		bundle->offset_tab[i][OFFSET] += (len - bundle->offset_tab[attr][STATE]);
//		PRINTF("BUNDLE: offset_tab[%u][OFFSET]=%u offset_tab[%u][STATE]=%u\n",i,bundle->offset_tab[i][OFFSET],i,bundle->offset_tab[i][STATE]);
	}
	bundle->size += (len - bundle->offset_tab[attr][STATE]);
	bundle->offset_tab[attr][STATE] = len;
	PRINTF("BUNDLE: offset_tab[%u][OFFSET]=%u offset_tab[%u][STATE]=%u\n",attr,bundle->offset_tab[attr][OFFSET],attr,bundle->offset_tab[attr][STATE]);
	uint8_t size=0;
	if (attr >2 && attr <17){
		for (i=3; i<17; i++){
			size+=bundle->offset_tab[i][STATE];
		}
		memset(bundle->block+bundle->offset_tab[LENGTH][OFFSET],size,1);
	}
	free(sdnv);
	sdnv=NULL;
#if DEBUG
	PRINTF("BUNDLE: bundle->size= %u\n",bundle->size);
	PRINTF("BUNDLE: SET_ATTR  %u %u : ",attr,bundle->offset_tab[DATA][OFFSET]);
	uint8_t* tmp= bundle->block;
	for (i=1;i<bundle->size;i++){
		PRINTF("%x ",*tmp);
		tmp++;
	}
	PRINTF("\n");
#endif
	return len;

}

uint8_t recover_bundel(struct bundle_t *bundle,struct mmem *mem, int size)
{
	uint8_t *block=(uint8_t *) MMEM_PTR(mem);
	PRINTF("rec bptr: %p  blptr:%p \n",bundle,block);
	bundle->offset_tab[VERSION][OFFSET]=0;
	bundle->offset_tab[VERSION][STATE]=1;
	bundle->offset_tab[FLAGS][OFFSET]=1;
	if (*block != 0){
		mmem_free(mem);
		block=NULL;
		return 0;
	}
	uint8_t *tmp=block;
	tmp+=1;
	uint8_t fields=0;
	if (*tmp & 0x1){ //fragmented	
		PRINTF("fragment\n");
		fields=16;
	}else{
		fields=14;
	}
	uint8_t i;
	for (i = 1; i<=fields; i++){
		uint8_t len= sdnv_len(tmp);
		bundle->offset_tab[i][STATE]=len;
		bundle->offset_tab[i][OFFSET]=(tmp-block);
		PRINTF("BUNDLE: RECOVER: %u: state: %u offset: %u value %u \n",i,bundle->offset_tab[i][STATE],bundle->offset_tab[i][OFFSET],*(block + bundle->offset_tab[i][OFFSET]) );
		tmp+=bundle->offset_tab[i][STATE];
		if(i==DIRECTORY_LEN && *(block + bundle->offset_tab[i][OFFSET]) != 0){
			PRINTF("\n\n NO CBHE %u\n\n",*(block + bundle->offset_tab[i][OFFSET]));
			mmem_free(mem);
			block=NULL;
			return 0;
		}

	}
	if (!(*tmp & 0x40)){ //not fragmented
		bundle->offset_tab[FRAG_OFFSET][OFFSET] = bundle->offset_tab[LIFE_TIME][OFFSET]+1;
		bundle->offset_tab[APP_DATA_LEN][OFFSET] = bundle->offset_tab[LIFE_TIME][OFFSET]+1;
	}
	bundle->offset_tab[DATA][STATE]= size- ((uint8_t)(tmp - block));
	PRINTF("BUNDLE: RECOVER: data size: %u=%u-%u\n",bundle->offset_tab[DATA][STATE], size,((uint8_t)(tmp - block)));
	sdnv_decode(block+bundle->offset_tab[LIFE_TIME][OFFSET],bundle->offset_tab[LIFE_TIME][STATE],&bundle->lifetime);
	bundle->offset_tab[DATA][OFFSET]= tmp-block;
	bundle->size=size;
	bundle->mem = (struct mmem *) malloc(sizeof(struct mmem));
	mmem_alloc(bundle->mem,size);
	bundle->block = (uint8_t *) MMEM_PTR(bundle->mem);
	if (bundle->block==NULL){
		PRINTF("\n\n MALLOC ERROR\n\n");
	}

	PRINTF("BUNDLE: RECOVER: block ptr: %p   ",bundle->offset_tab);
	for (i=0; i<27; i++){
		PRINTF("%u:",*(block+i));
	}
	PRINTF("\n");
	memcpy(bundle->block,block,size);
	mmem_free(mem);
	block=NULL;
	PRINTF("BUNDLE: RECOVERED\n");
	return 1;
}
uint16_t delete_bundle(struct bundle_t *bundle)
{
	
	PRINTF("fooooooooo %p\n",bundle->mem->ptr);
	mmem_free(bundle->mem);
	bundle->block=NULL;
	free(bundle->mem);
	bundle->mem=NULL;
	free(bundle);
	bundle=NULL;
}
