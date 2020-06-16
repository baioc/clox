#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h" // size_t

/* Custom allocator with the same protocol of stdlib's realloc, plus an extra
justification parameter. This will use the current Lox environment. */
void* reallocate(void* ptr, size_t size, const char* why);

// Recycles previously VM-allocated memory unused in the current Lox environment.
void collect_garbage(void);

#endif // CLOX_MEMORY_H
