#ifndef H_CLOX_VM
#define H_CLOX_VM

#include "chunk.h"
#include "common.h" // intptr_t
#include "value.h" // Value

// @TODO: determine needed stack size for each chunk during compilation
#define STACK_MAX 256

typedef struct {
	const Chunk* chunk;
	intptr_t pc;
	Value stack[STACK_MAX];
	Value* tos;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

// Initializes a Lox VM instance. vm_destroy() should be called on it later.
void vm_init(VM* vm);

// Deallocates any resources acquired by vm_init().
void vm_destroy(VM* vm);

// Executes VM's main entrypoint using CHUNK.
InterpretResult vm_interpret(VM* vm, const char* source);

#endif // H_CLOX_VM
