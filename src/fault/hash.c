#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fault/hash.h"
#include "fault/fault_list.h"

int calcHash(
	char* buffer			  /** バッファ */
)
{
	int hashval = 0;

	for (size_t i = 0; i < strlen(buffer); i++)
	{
		hashval += buffer[i];
	}

	return hashval % MAXSIZE_HASH;
}
