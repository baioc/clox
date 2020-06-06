#include "vm.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h> // strlen
#include <time.h> // clock(), CLOCKS_PER_SEC

#include <sgl/core.h> // ARRAY_SIZE

#include "chunk.h"
#include "value.h"
#include "object.h" // free_objects
#include "compiler.h"
#include "table.h"
#include "common.h"
#ifdef DEBUG_TRACE_EXECUTION
#	include "debug.h" // disassemble_instruction
#endif


static void reset_stack(VM* vm)
{
	vm->stack_pointer = vm->stack;
	vm->frame_count = 0;
}

static void define_native(VM* vm, const char* name, NativeFn function);

static Value native_clock(int argc, Value argv[])
{
	return number_value((double)clock() / CLOCKS_PER_SEC);
}

void vm_init(VM* vm)
{
	reset_stack(vm);
	vm->objects = NULL;
	table_init(&vm->globals);
	table_init(&vm->strings);
	define_native(vm, "clock", native_clock);
}

void vm_destroy(VM* vm)
{
	table_destroy(&vm->strings);
	table_destroy(&vm->globals);
	free_objects(&vm->objects);
}

static void push(VM* vm, Value value)
{
	*vm->stack_pointer = value;
	vm->stack_pointer++;
}

static Value pop(VM* vm)
{
	vm->stack_pointer--;
	return *vm->stack_pointer;
}

static Value peek(const VM* vm, int distance)
{
	return vm->stack_pointer[-1 - distance];
}

static bool value_is_falsey(Value value)
{
	return value_is_nil(value)
	    || (value_is_bool(value) && value_as_bool(value) == false);
}

static void concatenate_strings(VM* vm)
{
	const ObjString* b = value_as_string(pop(vm));
	const ObjString* a = value_as_string(pop(vm));
	ObjString* c = obj_string_concat(&vm->objects, &vm->strings, a, b);
	push(vm, obj_value((Obj*)c));
}

static void runtime_error(VM* vm, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	for (int i = vm->frame_count - 1; i >= 0; --i) {
		const CallFrame* frame = &vm->frames[i];
		const size_t instruction = frame->program_counter - 1;
		const int line = chunk_get_line(&frame->subroutine->bytecode, instruction);
		fprintf(stderr, "[line %d] in ", line);
		if (frame->subroutine->name == NULL)
			fprintf(stderr, "script\n");
		else
			fprintf(stderr, "%s()\n", frame->subroutine->name->chars);
	}

	reset_stack(vm);
}

// Should only be called during vm_init() !
static void define_native(VM* vm, const char* name, NativeFn function)
{
	push(vm, obj_value((Obj*)make_obj_string(&vm->objects, &vm->strings, name, strlen(name))));
	push(vm, obj_value((Obj*)make_obj_native(&vm->objects, function)));
	table_put(&vm->globals, value_as_string(vm->stack[0]), vm->stack[1]);
	pop(vm);
	pop(vm);
}

static bool call(VM* vm, ObjFunction* function, int argc)
{
	if (argc != function->arity) {
		runtime_error(vm, "Expected %d arguments but got %d.", function->arity, argc);
		return false;
	} else if (vm->frame_count >= ARRAY_SIZE(vm->frames)) {
		runtime_error(vm, "Stack overflow.");
		return false;
	} else {
		CallFrame* frame = &vm->frames[vm->frame_count++];
		frame->subroutine = function;
		frame->program_counter = 0;
		frame->frame_pointer = vm->stack_pointer - (argc + 1);
		return true;
	}
}

static bool call_value(VM* vm, Value callee, int argc)
{
	if (!value_is_obj(callee)) goto FAIL;

	switch (obj_type(callee)) {
		case OBJ_FUNCTION: return call(vm, value_as_function(callee), argc);
		case OBJ_NATIVE: {
			NativeFn native = value_as_native(callee);
			Value result = native(argc, vm->stack_pointer - argc);
			vm->stack_pointer -= argc + 1;
			push(vm, result);
			return true;
		}
		default: goto FAIL;
	}

FAIL:
	runtime_error(vm, "Can only call functions and classes.");
	return false;
}

static void debug_trace_run(const VM* vm, const CallFrame* frame)
{
#ifdef DEBUG_TRACE_EXECUTION
	printf(" /------> ");
	for (const Value* slot = vm->stack; slot < vm->stack_pointer; slot++) {
		printf("[ ");
		value_print(*slot);
		printf(" ]");
	}
	printf("\n");
	disassemble_instruction(&frame->subroutine->bytecode, frame->program_counter);
#endif
}

static InterpretResult run(VM* vm)
{
	CallFrame* frame = &vm->frames[vm->frame_count - 1];

	#define READ_BYTE() chunk_get_byte(&frame->subroutine->bytecode, frame->program_counter++)
	#define READ_CONSTANT() chunk_get_constant(&frame->subroutine->bytecode, READ_BYTE())
	#define READ_SHORT() \
		(frame->program_counter += 2, \
		 (chunk_get_byte(&frame->subroutine->bytecode, frame->program_counter - 2) << 8) \
		| chunk_get_byte(&frame->subroutine->bytecode, frame->program_counter - 1))
	#define READ_STRING() value_as_string(READ_CONSTANT())
	#define BINARY_OP(type_value, op) do { \
		if (!value_is_number(peek(vm, 0)) || !value_is_number(peek(vm, 1))) { \
			runtime_error(vm, "Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		const double b = value_as_number(pop(vm)); \
		const double a = value_as_number(pop(vm)); \
		push(vm, type_value(a op b)); \
	} while (0)

	// virtual machine fetch-decode-execute loop
	for (;;) {
		debug_trace_run(vm, frame);
		const uint8_t instruction = READ_BYTE();

		// @TODO: benchmark manual jump table with computed gotos
		switch (instruction) {
			case OP_CONSTANT: push(vm, READ_CONSTANT()); break;
			case OP_NIL: push(vm, nil_value()); break;
			case OP_TRUE: push(vm, bool_value(true)); break;
			case OP_FALSE: push(vm, bool_value(false)); break;
			case OP_POP: pop(vm); break;
			case OP_GET_LOCAL: {
				const uint8_t slot = READ_BYTE();
				push(vm, frame->frame_pointer[slot]);
				break;
			}
			case OP_SET_LOCAL: {
				const uint8_t slot = READ_BYTE();
				// assignment is an expression -> no need to pop assigned value
				frame->frame_pointer[slot] = peek(vm, 0);
				break;
			}
			case OP_GET_GLOBAL: {
				const ObjString* name = READ_STRING();
				Value value;
				if (!table_get(&vm->globals, name, &value)) {
					runtime_error(vm, "Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				push(vm, value);
				break;
			}
			case OP_DEFINE_GLOBAL: {
				const ObjString* name = READ_STRING();
				table_put(&vm->globals, name, peek(vm, 0));
				pop(vm);
				break;
			}
			case OP_SET_GLOBAL: {
				ObjString* name = READ_STRING();
				if (!table_put(&vm->globals, name, peek(vm, 0))) {
					table_delete(&vm->globals, name);
					runtime_error(vm, "Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}
			case OP_EQUAL: {
				const Value b = pop(vm);
				const Value a = pop(vm);
				push(vm, bool_value(value_equal(a, b)));
				break;
			}
			case OP_GREATER: BINARY_OP(bool_value, >); break;
			case OP_LESS: BINARY_OP(bool_value, <); break;
			case OP_ADD:
				if (value_is_string(peek(vm, 0)) && value_is_string(peek(vm, 1)))
					concatenate_strings(vm);
				else
					BINARY_OP(number_value, +);
				break;
			case OP_SUBTRACT: BINARY_OP(number_value, -); break;
			case OP_MULTIPLY: BINARY_OP(number_value, *); break;
			case OP_DIVIDE: BINARY_OP(number_value, /); break;
			case OP_NOT:
				push(vm, bool_value(value_is_falsey(pop(vm))));
				break;
			case OP_NEGATE:
				if (!value_is_number(peek(vm, 0))) {
					runtime_error(vm, "Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}
				push(vm, number_value(-value_as_number(pop(vm))));
				break;
			case OP_PRINT:
				value_print(pop(vm));
				printf("\n");
				break;
			case OP_JUMP: {
				const uint16_t jump = READ_SHORT();
				frame->program_counter += jump;
				break;
			}
			case OP_JUMP_IF_FALSE: {
				const uint16_t jump = READ_SHORT();
				if (value_is_falsey(peek(vm, 0)))
					frame->program_counter += jump;
				break;
			}
			case OP_LOOP: {
				const uint16_t jump = READ_SHORT();
				frame->program_counter -= jump;
				break;
			}
			case OP_CALL: {
				const int argc = READ_BYTE();
				const Value callee = peek(vm, argc);
				if (!call_value(vm, callee, argc)) {
					return INTERPRET_RUNTIME_ERROR;
				}

				frame = &vm->frames[vm->frame_count - 1];
				break;
			}
			case OP_RETURN: {
				const Value result = pop(vm);

				vm->frame_count--;
				if (vm->frame_count <= 0) {
					pop(vm);
					return INTERPRET_OK;
				}

				vm->stack_pointer = frame->frame_pointer;
				push(vm, result);

				frame = &vm->frames[vm->frame_count - 1];
				break;
			}
		}
	}

	#undef BINARY_OP
	#undef READ_STRING
	#undef READ_SHORT
	#undef READ_CONSTANT
	#undef READ_BYTE
}

InterpretResult vm_interpret(VM* vm, const char* source)
{
	// compile and store subroutine in the first stack slot (compiler-reserved)
	ObjFunction* program = compile(source, &vm->objects, &vm->strings);
	if (program == NULL) return INTERPRET_COMPILE_ERROR;

	// setup initial (i.e. main) call frame
	push(vm, obj_value((Obj*)program));
	call(vm, program, 0);

	return run(vm);
}
