#include "vm.h"

#include <stdio.h>

#include "value.h" // value_print
#include "common.h" // DEBUG_TRACE_EXECUTION
#include "debug.h" // disassemble_instruction


void vm_init(VM* vm)
{
	vm->tos = vm->stack;
}

void vm_destroy(VM* vm)
{}

static void stack_push(VM* vm, Value value)
{
	*(vm->tos) = value;
	vm->tos++;
}

static Value stack_pop(VM* vm)
{
	vm->tos--;
	return *vm->tos;
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
	#define BINARY_OP(op) do { \
		const Value b = stack_pop(vm); \
		const Value a = stack_pop(vm); \
		stack_push(vm, a op b); \
	} while (0)

	for (;;) {
		debug_trace_run(vm);
		const uint8_t instruction = READ_BYTE();
		switch (instruction) {
			case OP_RETURN:
				value_print(stack_pop(vm));
				printf("\n");
				return INTERPRET_OK;
			case OP_ADD:
				BINARY_OP(+);
				break;
			case OP_SUBTRACT:
				BINARY_OP(-);
				break;
			case OP_MULTIPLY:
				BINARY_OP(*);
				break;
			case OP_DIVIDE:
				BINARY_OP(/);
				break;
			case OP_NEGATE:
				stack_push(vm, -stack_pop(vm));
				break;
			case OP_CONSTANT:
				stack_push(vm, READ_CONSTANT());
				break;
		}
	}

	#undef BINARY_OP
	#undef READ_CONSTANT
	#undef READ_BYTE
}

InterpretResult vm_interpret(VM* vm, const Chunk* chunk)
{
	vm->chunk = chunk;
	vm->pc = 0;
	return run(vm);
}
