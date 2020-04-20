#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"
#include "common.h" // intptr_t

// Prints the contents of CHUNK called NAME in a human-readable format.
void disassemble_chunk(const Chunk* chunk, const char* name);

// Prints an instruction OFFSET bytes into CHUNK and returns its size.
int disassemble_instruction(const Chunk* chunk, intptr_t offset);

#endif // CLOX_DEBUG_H
