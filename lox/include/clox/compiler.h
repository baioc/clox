#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "common.h" // bool
#include "object.h" // Obj, ObjFunction
#include "table.h" // Table

/** Compiles null-terminated SOURCE Lox code to an ObjFunction, using auxiliary
 * structures OBJECTS and STRINGS to store static Lox objects.
 *
 * It should be noted that all string constants in STRINGS will be registered
 * to the compiled constant pool, and any new allocations for static objects
 * made during compilation will be added to list OBJECTS and table STRINGS.
 *
 * Returns non-NULL when successful. */
ObjFunction* compile(const char* source, Obj** objects, Table* strings);

#endif // CLOX_COMPILER_H
