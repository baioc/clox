#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "vm.h" // Environment
#include "object.h" // ObjFunction

/** Compiles null-terminated SOURCE Lox code to an ObjFunction using auxiliary
 * structures provided in DATA to store static Lox objects.
 *
 * Returns NULL on failure. */
ObjFunction* compile(const char* source, Environment* data);

#endif // CLOX_COMPILER_H
