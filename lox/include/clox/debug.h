#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h" // Chunk

// Prints the contents of CHUNK called NAME in a human-readable format.
void disassemble_chunk(const Chunk* chunk, const char* name);

#endif // CLOX_DEBUG_H
