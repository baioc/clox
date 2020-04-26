#include "vm.h"

#include <stdio.h>
#include <stdarg.h>

#include "chunk.h"
#include "value.h"
#include "object.h" // free_objects
#include "compiler.h"
#include "table.h"
#include "common.h" // NULL, DEBUG_TRACE_EXECUTION
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
	table_init(&vm->globals);
	table_init(&vm->strings);
}

void vm_destroy(VM* vm)
{
	table_destroy(&vm->strings);
	table_destroy(&vm->globals);
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
	ObjString* c = obj_string_concat(&vm->objects, &vm->strings, a, b);
	stack_push(vm, obj_value((Obj*)c));
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
	printf(" /------> ");
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
	#define READ_STRING() value_as_string(READ_CONSTANT())
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

	// virtual machine fetch-decode-execute loop
	for (;;) {
		debug_trace_run(vm);
		const uint8_t instruction = READ_BYTE();
		switch (instruction) {
			case OP_RETURN:   return INTERPRET_OK;
			case OP_CONSTANT: stack_push(vm, READ_CONSTANT()); break;
			case OP_NIL:      stack_push(vm, nil_value()); break;
			case OP_TRUE:     stack_push(vm, bool_value(true)); break;
			case OP_FALSE:    stack_push(vm, bool_value(false)); break;
			case OP_POP:      stack_pop(vm); break;
			case OP_GET_GLOBAL: {
				const ObjString* name = READ_STRING();
				Value value;
				if (!table_get(&vm->globals, name, &value)) {
					runtime_error(vm, "Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				stack_push(vm, value);
				break;
			}
			case OP_DEFINE_GLOBAL: {
				const ObjString* name = READ_STRING();
				table_put(&vm->globals, name, stack_peek(vm, 0));
				stack_pop(vm);
				break;
			}
			case OP_SET_GLOBAL: {
				ObjString* name = READ_STRING();
				if (!table_put(&vm->globals, name, stack_peek(vm, 0))) {
					table_delete(&vm->globals, name);
					runtime_error(vm, "Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}
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
			case OP_PRINT:
				value_print(stack_pop(vm));
				printf("\n");
				break;
		}
	}

	#undef BINARY_OP
	#undef READ_STRING
	#undef READ_CONSTANT
	#undef READ_BYTE
}

InterpretResult vm_interpret(VM* vm, const char* source)
{
	Chunk chunk;
	chunk_init(&chunk);

	// @NOTE: Objects created statically are only freed with the VM
	if (!compile(source, &chunk, &vm->objects, &vm->strings)) {
		chunk_destroy(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm->chunk = &chunk;
	vm->pc = 0;
	const InterpretResult result = run(vm);

	chunk_destroy(&chunk);
	return result;
}
