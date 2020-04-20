#include "vm.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> // malloc
#include <string.h> // memcpy

#include "value.h"
#include "memory.h" // free_objects
#include "object.h"
#include "chunk.h"
#include "compiler.h"
#include "common.h" // DEBUG_TRACE_EXECUTION, DEBUG_DYNAMIC_MEMORY
#ifdef DEBUG_TRACE_EXECUTION
#	include "debug.h" // disassemble_instruction
#endif


static void stack_reset(VM* vm)
{
	vm->tos = vm->stack;
}

void vm_init(VM* vm)
{
	stack_reset(vm);
	vm->objects = NULL;
}

void vm_destroy(VM* vm)
{
	free_objects(&vm->objects);
}

static void stack_push(VM* vm, Value value)
{
	*vm->tos = value;
	vm->tos++;
}

static Value stack_pop(VM* vm)
{
	vm->tos--;
	return *vm->tos;
}

static Value stack_peek(const VM* vm, int distance)
{
	return vm->tos[-1 - distance];
}

static bool value_is_falsey(Value value)
{
	return value_is_nil(value)
	    || (value_is_bool(value) && value_as_bool(value) == false);
}

static void concatenate_strings(VM* vm)
{
	const ObjString* b = value_as_string(stack_pop(vm));
	const ObjString* a = value_as_string(stack_pop(vm));

	// @TODO: optimize concatenation of immutable strings
	const int length = a->length + b->length;
	ObjString* result = make_obj_string(&vm->objects, length);
	memcpy(result->chars, a->chars, a->length);
	memcpy(result->chars + a->length, b->chars, b->length);
	result->chars[length] = '\0';

	stack_push(vm, obj_value((Obj*)result));
}

static void runtime_error(VM* vm, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	const int line = chunk_get_line(vm->chunk, vm->pc - 1);
	fprintf(stderr, "[line %d] in script\n", line);

	stack_reset(vm);
}

static void debug_trace_run(const VM* vm)
{
#ifdef DEBUG_TRACE_EXECUTION
	printf("          ");
	for (const Value* slot = vm->stack; slot < vm->tos; slot++) {
		printf("[ ");
		value_print(*slot);
		printf(" ]");
	}
	printf("\n");
	disassemble_instruction(vm->chunk, vm->pc);
#endif
}

static InterpretResult run(VM* vm)
{
	#define READ_BYTE() chunk_get_byte(vm->chunk, vm->pc++)
	#define READ_CONSTANT() chunk_get_constant(vm->chunk, READ_BYTE())
	#define BINARY_OP(type_value, op) do { \
		if (   !value_is_number(stack_peek(vm, 0)) \
		    || !value_is_number(stack_peek(vm, 1)) \
		   ) { \
			runtime_error(vm, "Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		const double b = value_as_number(stack_pop(vm)); \
		const double a = value_as_number(stack_pop(vm)); \
		stack_push(vm, type_value(a op b)); \
	} while (0)

	for (;;) {
		debug_trace_run(vm);
		const uint8_t instruction = READ_BYTE();
		switch (instruction) {
			case OP_RETURN:
				value_print(stack_pop(vm));
				printf("\n");
				return INTERPRET_OK;
			case OP_CONSTANT: stack_push(vm, READ_CONSTANT()); break;
			case OP_NIL:      stack_push(vm, nil_value()); break;
			case OP_TRUE:     stack_push(vm, bool_value(true)); break;
			case OP_FALSE:    stack_push(vm, bool_value(false)); break;
			case OP_EQUAL: {
				const Value b = stack_pop(vm);
				const Value a = stack_pop(vm);
				stack_push(vm, bool_value(value_equal(a, b)));
				break;
			}
			case OP_GREATER:  BINARY_OP(bool_value, >); break;
			case OP_LESS:     BINARY_OP(bool_value, <); break;
			case OP_ADD:
				if (value_is_string(stack_peek(vm, 0))
				 && value_is_string(stack_peek(vm, 1))
				   ) {
					concatenate_strings(vm);
				} else {
					BINARY_OP(number_value, +);
				}
				break;
			case OP_SUBTRACT: BINARY_OP(number_value, -); break;
			case OP_MULTIPLY: BINARY_OP(number_value, *); break;
			case OP_DIVIDE:   BINARY_OP(number_value, /); break;
			case OP_NOT:
				stack_push(vm, bool_value(value_is_falsey(stack_pop(vm))));
				break;
			case OP_NEGATE:
				if (!value_is_number(stack_peek(vm, 0))) {
					runtime_error(vm, "Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}
				stack_push(vm, number_value(-value_as_number(stack_pop(vm))));
				break;
		}
	}

	#undef BINARY_OP
	#undef READ_CONSTANT
	#undef READ_BYTE
}

InterpretResult vm_interpret(VM* vm, const char* source)
{
	Chunk chunk;
	chunk_init(&chunk);

	if (!compile(source, &chunk)) {
		chunk_destroy(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm->chunk = &chunk;
	vm->pc = 0;
	const InterpretResult result = run(vm);

	chunk_destroy(&chunk);
	return result;
}
