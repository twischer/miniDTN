#include <stdlib.h>
void * realloc ( void * ptr, size_t size ,size_t len)
{
	void *new = malloc(size);
	if (new==NULL){
		printf("\n\n MALLOC ERROR\n\n");
	}
//	printf("len %u \n",(*len-1));
	memcpy(new,ptr,len); //lentgh of a allocated area is in the first 7 bits at address pointer-1
	free(ptr);
	ptr=NULL;
	return new;
}
