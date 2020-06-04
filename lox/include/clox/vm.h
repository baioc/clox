#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "common.h" // intptr_t
#include "value.h" // Value
#include "object.h" // Obj
#include "table.h"

// @TODO: determine needed stack size for each chunk during comp. time
#define VM_STACK_MAX 256
// Lox bytecode virtual machine.
typedef struct {
	const Chunk* chunk;
	intptr_t pc;
	Value stack[VM_STACK_MAX];
	Value* tos;
	Obj* objects;
	Table globals;
	Table strings;
} VM;
#undef VM_STACK_MAX

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

#endif // CLOX_VM_H
