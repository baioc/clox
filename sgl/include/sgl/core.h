#ifndef H_SGL_CORE
#define H_SGL_CORE

#include <stddef.h> // size_t, NULL
#include <stdbool.h> // bool
#include <stdlib.h> // free

typedef unsigned char byte_t;
typedef int err_t;

// Gets the size (number of elements) of static array ARR.
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Swaps the memory contents of the first SIZE bytes pointed to by A and B.
void swap(void *a, void *b, size_t size);

// Calls free() on the pointer pointed to by PTR and sets it to NULL.
inline void freeref(void** ptr)
{
	free(*ptr);
	*ptr = NULL;
}

#endif // H_SGL_CORE
