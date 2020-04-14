#include "core.h"

#include <string.h> // memcpy

void swap(void *a, void *b, size_t size)
{
	byte_t tmp[size];
	memcpy(tmp, a, size);
	memcpy(a, b, size);
	memcpy(b, tmp, size);
}
