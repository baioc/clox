#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h" // size_t
#include "vm.h" // VM

// Custom allocator with the same protocol of stdlib's realloc plus justification.
void* reallocate(void* ptr, size_t size, const char* why);

// Recycles previous VM-allocated, Lox-unreachable memory.
void collect_garbage(VM* vm);

#endif // CLOX_MEMORY_H
