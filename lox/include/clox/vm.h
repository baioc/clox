#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "common.h" // intptr_t, UINT8_MAX
#include "value.h" // Value
#include "object.h" // Obj, ObjFunction
#include "table.h"

typedef struct {
	intptr_t program_counter;
	Value* frame_pointer;
	ObjFunction* subroutine;
} CallFrame;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * (UINT8_MAX + 1))

// Lox bytecode virtual machine.
typedef struct {
	CallFrame frames[FRAMES_MAX];
	int frame_count;
	Value stack[STACK_MAX];
	Value* stack_pointer;
	Obj* objects;
	Table globals;
	Table strings;
} VM;

#undef STACK_MAX
#undef FRAMES_MAX

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
