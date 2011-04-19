#include <stdlib.h>
void * realloc ( void * ptr, size_t size )
{
	void *new = malloc(size);
	size_t *len = (size_t *)ptr-1;
	printf("len %u \n",(*len-1));
	memcpy(new,ptr,(*len-1)); //lentgh of a allocated area is in the first 7 bits at address pointer-1
	free(ptr);
	return new;
}
