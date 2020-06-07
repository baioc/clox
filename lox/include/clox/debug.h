#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"
#include "common.h" // intptr_t
#include "value.h" // ValueArray

/* Prints the contents of CHUNK called NAME in a human-readable format using
extra information from the provided CONSTANTS pool. */
void disassemble_chunk(const Chunk* chunk, const ValueArray* constants, const char* name);

// Prints an instruction OFFSET bytes into CHUNK (usign CONSTANTS) and returns its size.
int disassemble_instruction(const Chunk* chunk, const ValueArray* constants, intptr_t offset);

#endif // CLOX_DEBUG_H
