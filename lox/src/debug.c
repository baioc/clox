#include "debug.h"

#include <stdio.h>

#include "chunk.h"
#include "common.h" // uint8_t
#include "value.h" // value_print
#include "vm.h" // constant_get


static int constant_instruction(const char* name, const Chunk* chunk, intptr_t addr,
                                const ValueArray* constants)
{
	const int8_t idx = chunk_get_byte(chunk, addr + 1);
	printf("%-16s %4d '", name, idx);
	value_print(constant_get(constants, idx));
	printf("'\n");
	return 2;
}

static int simple_instruction(const char* name)
{
	printf("%s\n", name);
	return 1;
}

static int byte_instruction(const char* name, const Chunk* chunk, intptr_t addr)
{
	const uint8_t slot = chunk_get_byte(chunk, addr + 1);
	printf("%-16s %4d\n", name, slot);
	return 2;
}

static int jump_instruction(const char* op, const Chunk* chk, int addr, int sgn)
{
	uint16_t jump = chunk_get_byte(chk, addr + 1) << 8;
	jump |= chunk_get_byte(chk, addr + 2);
	printf("%-16s %4d -> %d\n", op, addr, addr + 3 + sgn * jump);
	return 3;
}

int disassemble_instruction(const Chunk* chunk, const ValueArray* constants, intptr_t offset)
{
	#define CASE_SIMPLE(opcode) \
		case opcode: return simple_instruction(#opcode)
	#define CASE_CONSTANT(opcode) \
		case opcode: return constant_instruction(#opcode, chunk, offset, constants)
	#define CASE_BYTE(opcode) \
		case opcode: return byte_instruction(#opcode, chunk, offset)
	#define CASE_JUMP(opcode, sign) \
		case opcode: return jump_instruction(#opcode, chunk, offset, (sign))

	// print byte address and line number
	printf("%04d ", offset);
	const int line = chunk_get_line(chunk, offset);
	if (offset > 0 && line == chunk_get_line(chunk, offset - 1))
		printf("   | ");
	else
		printf("%4d ", line);

	// print instruction accordingly
	const uint8_t instruction = chunk_get_byte(chunk, offset);
	switch (instruction) {
		CASE_CONSTANT(OP_CONSTANT);
		CASE_SIMPLE(OP_NIL);
		CASE_SIMPLE(OP_TRUE);
		CASE_SIMPLE(OP_FALSE);
		CASE_SIMPLE(OP_POP);
		CASE_BYTE(OP_GET_LOCAL);
		CASE_BYTE(OP_SET_LOCAL);
		CASE_CONSTANT(OP_GET_GLOBAL);
		CASE_CONSTANT(OP_DEFINE_GLOBAL);
		CASE_CONSTANT(OP_SET_GLOBAL);
		CASE_BYTE(OP_GET_UPVALUE);
		CASE_BYTE(OP_SET_UPVALUE);
		CASE_SIMPLE(OP_EQUAL);
		CASE_SIMPLE(OP_GREATER);
		CASE_SIMPLE(OP_LESS);
		CASE_SIMPLE(OP_ADD);
		CASE_SIMPLE(OP_SUBTRACT);
		CASE_SIMPLE(OP_MULTIPLY);
		CASE_SIMPLE(OP_DIVIDE);
		CASE_SIMPLE(OP_NOT);
		CASE_SIMPLE(OP_NEGATE);
		CASE_SIMPLE(OP_PRINT);
		CASE_JUMP(OP_JUMP, 1);
		CASE_JUMP(OP_JUMP_IF_FALSE, 1);
		CASE_JUMP(OP_LOOP, -1);
		CASE_BYTE(OP_CALL);
		case OP_CLOSURE: {
			offset++;
			const uint8_t id = chunk_get_byte(chunk, offset++);
			printf("%-16s %4d ", "OP_CLOSURE", id);
			Value closure = constant_get(constants, id);
			value_print(closure);
			printf("\n");

			ObjFunction* function = value_as_function(closure);
			for (int i = 0; i < function->upvalues; ++i) {
				const int local = chunk_get_byte(chunk, offset++);
				const int index = chunk_get_byte(chunk, offset++);
				printf("%04d      |                     %s %d\n",
				       offset - 2, local ? "local" : "upvalue", index);
			}

			return offset;
		}
		CASE_SIMPLE(OP_CLOSE_UPVALUE);
		CASE_SIMPLE(OP_RETURN);
		default: printf("Unknown opcode %d\n", instruction); return 1;
	}

	#undef CASE_JUMP
	#undef CASE_BYTE
	#undef CASE_CONSTANT
	#undef CASE_SIMPLE
}

void disassemble_chunk(const Chunk* chunk, const ValueArray* constants, const char* name)
{
	printf("== %s ==\n", name);

	printf(" .data\n");
	for (int i = 0; i < value_array_size(constants); ++i) {
		printf("%04d      ", i);
		value_print(value_array_get(constants, i));
		printf("\n");
	}

	printf(" .text\n");
	for (intptr_t offset = 0; offset < chunk_size(chunk);)
		offset += disassemble_instruction(chunk, constants, offset);
}
