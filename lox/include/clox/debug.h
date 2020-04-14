#ifndef H_CLOX_DEBUG
#define H_CLOX_DEBUG

#include "chunk.h" // chunk_t
#include "common.h" // intptr_t

// Prints the contents of CHUNK (named NAME) in a human-readable format.
void disassemble_chunk(const chunk_t* chunk, const char* name);

#endif // H_CLOX_DEBUG
