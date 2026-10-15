#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/memory.h"

void* allocMemory(
	size_t			      count,			  /**< 確保する要素数 */
	size_t			      size				  /**< 型のサイズ */
)
{
	void* ptr = (void*)NULL;


	if ((ptr = calloc(count, size)) == (void*)NULL)
	{
		printf("\n	MEMORY ERROR: memory allocation could not be secured.\n");

		exit(EXIT_FAILURE);
	}

	return	ptr;
}
