#ifndef CLOX_VM_H
#define CLOX_VM_H

#include <ugly/stack.h>

#include "chunk.h"
#include "common.h" // intptr_t, UINT8_MAX, size_t
#include "value.h" // Value, ValueArray
#include "object.h" // Obj, ObjFunction
#include "table.h"


// A data container for the information needed during subroutine execution.
typedef struct {
	ObjClosure* subroutine;
	intptr_t program_counter;
	Value* frame_pointer;
} CallFrame;

// forward decls. for environment's GC info
struct VM;
struct Compiler;

// Environment structure acting as the VM's heap and data segments.
typedef struct {
	// GC info
	size_t allocated;
	size_t next_gc;
	stack_t grays;
	struct Compiler* compiler;
	struct VM* vm;
	// heap data
	Table globals;
	ObjUpvalue* open_upvalues;
	Obj* objects;
	Table strings;
	// data segment
	ValueArray constants;
} Environment;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * (UINT8_MAX + 1))

// Lox bytecode virtual machine.
typedef struct VM {
	int frame_count;
	CallFrame frames[FRAMES_MAX];
	Value* stack_pointer;
	Value stack[STACK_MAX];
	Environment data;
	ObjString* init_string;
} VM;

#undef STACK_MAX
#undef FRAMES_MAX

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;


// Gets the current lox execution environment, or NULL if uninitialized.
Environment* lox_getenv(void);

// Defines the current lox execution environment as ENV, returning the old one.
Environment* lox_setenv(Environment* env);

/** Adds a constant VALUE to the CONSTANT pool.
 * Returns the pool index where it was added for later access.
 * @NOTE: Because OP_CONSTANT only uses a single byte for its operand, a
 * constant pool has a maximum capacity of 256 elements.*/
int constant_add(ValueArray* constants, Value value);

// Gets the constant value added to INDEX of the CONSTANT pool.
Value constant_get(const ValueArray* constants, uint8_t index);

// Initializes a Lox VM instance. vm_destroy() should be called on it later.
void vm_init(VM* vm);

// Deallocates any resources acquired by vm_init().
void vm_destroy(VM* vm);

// Executes the VM over the given SOURCE Lox program.
InterpretResult vm_interpret(VM* vm, const char* source);

#endif // CLOX_VM_H
