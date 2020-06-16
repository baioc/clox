#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "vm.h" // Environment
#include "object.h" // ObjFunction
#include "scanner.h" // Token
#include "common.h" // uint8_t

typedef struct {
	Token name;
	int depth;
	bool captured;
} Local;

typedef struct {
	uint8_t index;
	bool local;
} Upvalue;

typedef enum {
	TYPE_FUNCTION,
	TYPE_SCRIPT,
} FunctionType;

// Compilation context.
typedef struct Compiler {
	FunctionType type;
	ObjFunction* subroutine;
	struct Compiler* enclosing;
	int scope_depth;
	int local_count;
	Local locals[UINT8_MAX + 1];
	Upvalue upvalues[UINT8_MAX + 1];
} Compiler;

/** Compiles null-terminated SOURCE Lox code to an ObjFunction using auxiliary
 * structures provided in DATA to store static Lox objects.
 *
 * Returns NULL on failure. */
ObjFunction* compile(const char* source, Environment* data);

#endif // CLOX_COMPILER_H
