#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h" // size_t
#include "vm.h" // Environment

// GC allocator following realloc's protocol, with added ENV and WHY parameters.
void* reallocate(Environment* env, void* ptr, size_t size, const char* why);

// GC collector.
void collect_garbage(Environment *env);

#endif // CLOX_MEMORY_H
