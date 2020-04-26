#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "common.h" // bool
#include "chunk.h" // Chunk
#include "object.h" // Obj
#include "table.h" // Table

/** Compiles null-terminated SOURCE Lox code to given CHUNK, using auxiliary
 * structures OBJECTS and STRINGS to store static Lox objects.
 *
 * It should be noted that all string constants in STRINGS will be registered
 * to the CHUNK's constant pool, and any new allocations for static objects
 * made during compilation will be added to list OBJECTS and table STRINGS.
 *
 * Returns true when successful. */
bool compile(const char* source, Chunk* chunk, Obj** objects, Table* strings);

#endif // CLOX_COMPILER_H
