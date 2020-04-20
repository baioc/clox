#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "common.h" // bool
#include "chunk.h" // Chunk

// Compiles null-terminated SOURCE to a CHUNK. Returns true when successful.
bool compile(const char* source, Chunk* chunk);

#endif // CLOX_COMPILER_H
