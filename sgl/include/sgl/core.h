#ifndef H_SGL_CORE
#define H_SGL_CORE

#include <stddef.h> // size_t, NULL
#include <stdbool.h> // bool

typedef unsigned char byte_t;
typedef int err_t;

// Swaps the memory contents of the first SIZE bytes pointed to by A and B.
void swap(void *a, void *b, size_t size);

#endif // H_SGL_CORE
