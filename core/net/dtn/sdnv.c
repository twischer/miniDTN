#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>


#define MAX_LENGTH 8 

int sdnv_encode(uint64_t val, uint8_t* bp, size_t len)
{
	size_t val_len = 0;
	uint64_t tmp = val;

	do {
		tmp = tmp >> 7;
		val_len++;
	} while (tmp != 0);


	if (len < val_len) {
		return -1;
	}

	bp += val_len;
	uint8_t high_bit = 0; // for the last octet
	do {
		--bp;
		*bp = (uint8_t)(high_bit | (val & 0x7f));
		high_bit = (1 << 7); // for all but the last octet
		val = val >> 7;
	} while (val != 0);

	return val_len;
}

size_t sdnv_encoding_len(uint64_t val)
{
	size_t val_len = 0;
	do {
		val = val >> 7;
		val_len++;
	} while (val != 0);
	
	return val_len;
}

int sdnv_decode(const uint8_t* bp, size_t len, uint64_t* val)
{
	const uint8_t* start = bp;
	if (!val) {
		return -1;
	}
	size_t val_len = 0;
	*val = 0;
	do {
		if (len == 0){
			return -1; // buffer too short
		}
		*val = (*val << 7) | (*bp & 0x7f);
		++val_len;

		if ((*bp & (1 << 7)) == 0){
			break; // all done;
		}

		++bp;
		--len;
	} while (1);

	if ((val_len > MAX_LENGTH) || ((val_len == MAX_LENGTH) && (*start != 0x81)))
		return -1;
	
	return val_len;
}

size_t sdnv_len(const uint8_t* bp)
{
	size_t val_len = 1;
	for ( ; *bp++ & 0x80; ++val_len )
		;
	return val_len;
}



#if 0
int main(void)
{
	printf("hello\n");
	uint8_t *sdnv;
	size_t len= sdnv_encoding_len(0xABC);
	sdnv=(uint8_t *) malloc(len);
	printf("hello\n");
	int foo=sdnv_encode(0xABC,sdnv,len);
	int i;
	for(i=0; i<len; i++){
		printf("%x:",*(sdnv+i));
	}
	printf("\n");
	uint64_t val;
	sdnv_decode(sdnv,len,&val);
	printf("%lx\n",val);


}
#endif
