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



void mmem_realloc(struct mmem * mem, int oldsize, int size) {
	struct mmem mmem_tmp;
	mmem_alloc(&mmem_tmp, size);
	memcpy(mmem_tmp.ptr, mem->ptr, oldsize);
	mmem_free(mem);
	memset(mem, 0, sizeof(struct mmem));
	memcpy(mem, &mmem_tmp, sizeof(struct mmem));
}

/** 
* brief creates a new bundle and allocates the minimum needed memory
*/
uint8_t create_bundle(struct bundle_t *bundle)
{
	memset(bundle, 0, sizeof(struct bundle_t));

	bundle->offset_tab[VERSION][OFFSET]=0;
	bundle->offset_tab[FLAGS][OFFSET]=1;

/*
	bundle->mem = (struct mmem*) malloc(sizeof(struct mmem));
	if (!bundle->mem){
		printf(" \n\n BUNDLE ERROR\n\n");
		while(1) ;	
		return 0;
	}
	memset(bundle->mem, 0, sizeof(struct mmem));

	mmem_alloc(bundle->mem,1);
	// bundle->mem.ptr = (uint8_t *) MMEM_PTR(bundle->mem);
	if (bundle->mem.ptr==NULL){
		PRINTF("\n\n MALLOC ERROR\n\n");
		while(1) ;	
	}
*/
	if( !mmem_alloc(&bundle->mem, 1) ) {
		PRINTF("\n\n MALLOC ERROR\n\n");
		while(1) ;	
	}

	/*
	if (bundle->mem.ptr==NULL){
		PRINTF("\n\n MALLOC ERROR\n\n");
		while(1) ;	
	}
	*/

	//*bundle->mem.ptr = 0;
	bundle->size=1;
	uint8_t i;
	bundle->rec_time=(uint32_t) clock_seconds(); 
	bundle->offset_tab[VERSION][STATE]=1;
	for (i=1;i<18;i++){
		bundle->offset_tab[i][OFFSET]=1;
		bundle->offset_tab[i][STATE]=0;
	}
	//memcpy(bundle->mem.ptr + 2, payload, len);
	//bundle->offset_tab[PAYLOAD][STATE] = len;
	//uint8_t *tmp=bundle->mem.ptr;
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
	PRINTF("BUNDLE: set len\n");
	i=set_attr(bundle, LENGTH, &len64);
	PRINTF("BUNDLE: set dir_len\n");
	i+=set_attr(bundle, DIRECTORY_LEN, &len64);
//	bundle->size += len64 + i; 
#if DEBUG
	PRINTF("BUNDLE: CREATE ");
	uint8_t* tmp= bundle->mem.ptr;
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
//	sdnv_t s_len;
	size_t len = sdnv_encoding_len((uint32_t )d_len);
	struct mmem mem;
	mmem_alloc(&mem,len);
//	s_len = (uint8_t *) malloc(len);
//	if (s_len==NULL){
//		PRINTF("\n\n MALLOC ERROR\n\n");
//		while(1);
//	}

	sdnv_encode((uint32_t) d_len, (sdnv_t) mem.ptr, len);

	/*
	struct mmem *mmem_tmp = (struct mmem*) malloc(sizeof(struct mmem));

		if (!mmem_tmp){
			printf(" \n\n BUNDLE ERROR\n\n");
			return 0;
		}
	mmem_alloc(mmem_tmp,d_len + len + 2  + bundle->size);
	memcpy(mmem_tmp->ptr,bundle->mem.ptr,bundle->size);
	mmem_free(bundle->mem);
	free(bundle->mem),
	bundle->mem=mmem_tmp;
	*/

	struct mmem mmem_tmp;
	mmem_alloc(&mmem_tmp, d_len + len + 2  + bundle->size);
	memcpy((uint8_t*)mmem_tmp.ptr, (uint8_t*) bundle->mem.ptr, bundle->size);
	mmem_free(&bundle->mem);
	memset(&bundle->mem, 0, sizeof(struct mmem));
	memcpy(&bundle->mem, &mmem_tmp, sizeof(mmem_tmp));
	mmem_reorg(&mmem_tmp,&bundle->mem);

	/*
	mmem_realloc(&bundle->mem, bundle->size, d_len + len + 2  + bundle->size);
	*/

	/*
	struct mmem mmem_tmp;
	mmem_alloc(&mmem_tmp,d_len + len + 2  + bundle->size);
	memcpy(mmem_tmp.ptr,bundle->mem.ptr,bundle->size);
	mmem_free(&bundle->mem);
	memset(&bundle->mem, 0, sizeof(struct mmem));
	memcpy(&bundle->mem, &mmem_tmp, sizeof(struct mmem));
	*/

/*#if CONTIKI_TARGET_SKY
	bundle->mem.ptr = (uint8_t *) realloc(bundle->mem.ptr,d_len + len + 2  + bundle->size,bundle->size);
#else
	bundle->mem.ptr = (uint8_t *) realloc(bundle->mem.ptr,d_len + len + 2  + bundle->size);
#endif
*/
	if (bundle->mem.ptr == NULL) {
		return 0;
	}
	memcpy((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE], &type, 1);
	bundle->offset_tab[DATA][STATE] +=1;
	memcpy((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE], &flags, 1);
	bundle->offset_tab[DATA][STATE] +=1;
	memcpy((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE], (uint8_t*)mem.ptr, len);
	bundle->offset_tab[DATA][STATE] +=len;
	memcpy((uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE],data,d_len);
	bundle->offset_tab[DATA][STATE] +=d_len;
	bundle->size = bundle->offset_tab[DATA][OFFSET] + bundle->offset_tab[DATA][STATE]; 
	mmem_free(&mem);
#if DEBUG
	uint8_t *tmp= (uint8_t*)bundle->mem.ptr + bundle->offset_tab[DATA][OFFSET];
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
void hexdump(char * string, uint8_t * bla, int length) {
#if DEBUG
	printf("Hex: (%s) ", string);
	int i;
	for(i=0; i<length; i++) {
		printf("%02X ", *(bla+i));
	}
	printf("\n");
#endif
}

uint8_t set_attr(struct bundle_t *bundle, uint8_t attr, uint32_t *val)
{
	PRINTF("set attr %lx\n",*val);
	if (attr == FLAGS){
		bundle->custody = 0x08 &(uint8_t) *val;
	}
	if( attr == LIFE_TIME){
		bundle->lifetime= *val;
	}
	//sdnv_t sdnv;
	size_t len = sdnv_encoding_len(*val);
//	printf("tpr %u\n ",len);  // this fixes everything
	//sdnv = (uint8_t *) malloc(len);
	uint8_t sdnv[30];
	/*
	if (sdnv==NULL){
		PRINTF("\n\n MALLOC ERROR\n\n");
	}
	*/

	sdnv_encode(*val,sdnv,len);

	if(((int16_t)(len-bundle->offset_tab[attr][STATE])) > 0){
		PRINTF("BUNDLE: realloc (%u-%u)+%u= %u\n",len,bundle->offset_tab[attr][STATE],bundle->mem.size,(len-bundle->offset_tab[attr][STATE]) + bundle->mem.size);

		hexdump("vorher  ", bundle->mem.ptr, bundle->mem.size);
		struct mmem mmem_tmp ;
		if(!mmem_alloc(&mmem_tmp,(len-bundle->offset_tab[attr][STATE]) + bundle->mem.size)){
			PRINTF("MMEM ERROR\n");
			while(1);
		}
		hexdump("vorher  ", bundle->mem.ptr, bundle->mem.size);
		//memset((uint8_t*)mmem_tmp.ptr,7,mmem_tmp.size);
		hexdump("vorher  ", bundle->mem.ptr, bundle->mem.size);
		PRINTF("cpy: %u  %p --> %p\n", bundle->offset_tab[attr][OFFSET],bundle->mem.ptr,mmem_tmp.ptr);
		memcpy((uint8_t*)mmem_tmp.ptr,(uint8_t*)bundle->mem.ptr,bundle->offset_tab[attr][OFFSET]);
		hexdump("memcpy 1", mmem_tmp.ptr, mmem_tmp.size);
		
		memcpy((uint8_t*)mmem_tmp.ptr + bundle->offset_tab[attr][OFFSET]+(len-bundle->offset_tab[attr][STATE]), (uint8_t*)bundle->mem.ptr + bundle->offset_tab[attr][OFFSET], bundle->size - bundle->offset_tab[attr][OFFSET]);
		hexdump("memcpy 2", mmem_tmp.ptr, mmem_tmp.size);
		PRINTF("mmem_tmp.ptr %p %p\n",mmem_tmp.ptr,mmem_tmp.next);
		mmem_free(&bundle->mem);
		PRINTF("mmem_tmp.ptr %p\n",MMEM_PTR(&mmem_tmp));
		memcpy(&bundle->mem, &mmem_tmp, sizeof(mmem_tmp));
		mmem_reorg(&mmem_tmp,&bundle->mem);
		PRINTF("bundle->mem.ptr %p\n",bundle->mem.ptr);
		/*
		struct mmem *mmem_tmp = (struct mmem*) malloc(sizeof(struct mmem));
		if (!mmem_tmp){
			printf(" \n\n BUNDLE ERROR\n\n");
			return 0;
		}
		mmem_alloc(mmem_tmp,(len-bundle->offset_tab[attr][STATE]) + bundle->size);
		memcpy(mmem_tmp->ptr,bundle->mem.ptr,bundle->offset_tab[attr][OFFSET]);
		memcpy(mmem_tmp->ptr + bundle->offset_tab[attr][OFFSET]+(len-bundle->offset_tab[attr][STATE]), bundle->mem.ptr + bundle->offset_tab[attr][OFFSET], bundle->size - bundle->offset_tab[attr][OFFSET]);
		mmem_free(bundle->mem);
		free(bundle->mem),
		bundle->mem=mmem_tmp;
		*/

		/*
#if CONTIKI_TARGET_SKY
		bundle->mem.ptr = (uint8_t *) realloc(bundle->mem.ptr,((int16_t)(len-bundle->offset_tab[attr][STATE])) + bundle->size,bundle->size);
		PRINTF("BUNDLE: mem-size %u\n",((int16_t)(len-bundle->offset_tab[attr][STATE])) + bundle->size);
#else
		bundle->mem.ptr = (uint8_t *) realloc(bundle->mem.ptr,((int16_t)(len-bundle->offset_tab[attr][STATE])) + bundle->size);
#endif
*/
	//	memmove((bundle->mem.ptr + bundle->offset_tab[attr][OFFSET] + len), bundle->mem.ptr + bundle->offset_tab[attr][OFFSET], bundle->size - bundle->offset_tab[attr][OFFSET] );
	}
	if (((int16_t)(len-bundle->offset_tab[attr][STATE])) < 0){
		PRINTF("BUNDLE: smaller\n");
		struct mmem mmem_tmp ;
		mmem_alloc(&mmem_tmp,bundle->size + ((int16_t)(len-bundle->offset_tab[attr][STATE])));
		memcpy((uint8_t*)mmem_tmp.ptr,(uint8_t*)bundle->mem.ptr,bundle->offset_tab[attr][OFFSET]);
		memcpy((uint8_t*)mmem_tmp.ptr + bundle->offset_tab[attr][OFFSET] + len,(uint8_t*)bundle->mem.ptr + bundle->offset_tab[attr][OFFSET] + bundle->offset_tab[attr][STATE],bundle->size + ((int16_t)(len-bundle->offset_tab[attr][STATE])) );
		mmem_free(&bundle->mem);
		memcpy(&bundle->mem, &mmem_tmp, sizeof(mmem_tmp));
		mmem_reorg(&mmem_tmp,&bundle->mem);
		
		
		/*struct mmem *mmem_tmp = (struct mmem*) malloc(sizeof(struct mmem));
		if (!mmem_tmp){
			printf(" \n\n BUNDLE ERROR\n\n");
			return 0;
		}
		mmem_alloc(mmem_tmp,bundle->size + ((int16_t)(len-bundle->offset_tab[attr][STATE])));
		uint8_t *tmp=(uint8_t*) MMEM_PTR(mmem_tmp);
		if (*tmp==NULL){
			PRINTF("\n\n MALLOC ERROR\n\n");
		}

		memcpy(tmp,bundle->mem.ptr,bundle->offset_tab[attr][OFFSET]);
		memcpy(tmp + bundle->offset_tab[attr][OFFSET] + len,bundle->mem.ptr + bundle->offset_tab[attr][OFFSET] + bundle->offset_tab[attr][STATE],bundle->size + ((int16_t)(len-bundle->offset_tab[attr][STATE])) );
		mmem_free(bundle->mem);
		free(bundle->mem);
		bundle->mem=mmem_tmp;

		bundle->mem.ptr=tmp;
		*/
	}
	PRINTF("%p + %d = %p -> %u (%d)\n", bundle->mem.ptr, bundle->offset_tab[attr][OFFSET], (bundle->mem.ptr + bundle->offset_tab[attr][OFFSET]), sdnv, len);
	memcpy((uint8_t*)bundle->mem.ptr + bundle->offset_tab[attr][OFFSET], sdnv, len);
	hexdump("memcpy 3", bundle->mem.ptr, bundle->mem.size);

	uint8_t i;
	PRINTF("BUNDLE: val= %lu\n",*val);
	for(i=attr+1;i<18;i++){
		bundle->offset_tab[i][OFFSET] += (len - bundle->offset_tab[attr][STATE]);
//		PRINTF("BUNDLE: offset_tab[%u][OFFSET]=%u offset_tab[%u][STATE]=%u\n",i,bundle->offset_tab[i][OFFSET],i,bundle->offset_tab[i][STATE]);
	}
	PRINTF("bundle->size %u+(%u-%u)\n",bundle->size,len,bundle->offset_tab[attr][STATE]);
	bundle->size += (len - bundle->offset_tab[attr][STATE]);
	bundle->offset_tab[attr][STATE] = len;
	PRINTF("BUNDLE: offset_tab[%u][OFFSET]=%u offset_tab[%u][STATE]=%u\n",attr,bundle->offset_tab[attr][OFFSET],attr,bundle->offset_tab[attr][STATE]);
	uint8_t size=0;
	if (attr >2 && attr <17){
		for (i=3; i<17; i++){
			size+=bundle->offset_tab[i][STATE];
		}
		PRINTF("set new len %u\n",size);
		memset((uint8_t*)bundle->mem.ptr+bundle->offset_tab[LENGTH][OFFSET],size,1);
	}
	// free(sdnv);
	// sdnv=NULL;
#if DEBUG
	PRINTF("BUNDLE: bundle->size= %u\n",bundle->size);
	PRINTF("BUNDLE: SET_ATTR  %u %u : ",attr,bundle->offset_tab[DATA][OFFSET]);
	uint8_t* tmp= bundle->mem.ptr;
	for (i=1;i<bundle->size;i++){
		PRINTF("%x ",*tmp);
		tmp++;
	}
	PRINTF("\n");
#endif
	hexdump("ende   ", bundle->mem.ptr, bundle->mem.size);
	PRINTF("\n");	
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
	sdnv_decode(block+bundle->offset_tab[FLAGS][OFFSET],bundle->offset_tab[FLAGS][STATE],&bundle->flags);
	bundle->offset_tab[DATA][OFFSET]= tmp-block;
	bundle->size=size;
	/*bundle->mem = (struct mmem *) malloc(sizeof(struct mmem));
		if (!bundle->mem){
			printf(" \n\n BUNDLE ERROR\n\n");
			return 0;
		}
	*/
	mmem_alloc(&bundle->mem,size);
	//bundle->mem.ptr = (uint8_t *) MMEM_PTR(bundle->mem);
	//if (bundle->mem.ptr==NULL){
	//	PRINTF("\n\n MALLOC ERROR\n\n");
	//}

	PRINTF("BUNDLE: RECOVER: block ptr: %p   ",bundle->offset_tab);
	for (i=0; i<27; i++){
		PRINTF("%u:",*(block+i));
	}
	PRINTF("\n");
	memcpy((uint8_t*)bundle->mem.ptr,(uint8_t*)block,size);
	mmem_free(mem);
	block=NULL;
	PRINTF("BUNDLE: RECOVERED\n");
	return 1;
}
uint16_t delete_bundle(struct bundle_t *bundle)
{
	
	PRINTF("BUNDLE: bundle->mem.ptr %p\n",bundle->mem.ptr);
	mmem_free(&bundle->mem);
	//bundle->mem.ptr=NULL;
	//free(bundle->mem);
	//bundle->mem=NULL;
//	free(bundle);
//	bundle=NULL;
}
