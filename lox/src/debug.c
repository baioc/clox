#include "debug.h"

#include <stdio.h>

#include "chunk.h"
#include "common.h" // uint8_t
#include "value.h" // value_print


static int constant_instruction(const char* op, const Chunk* chk, intptr_t addr)
{
	const int8_t const_idx = chunk_get_byte(chk, addr + 1);
	printf("%-16s %4d '", op, const_idx);
	value_print(chunk_get_constant(chk, const_idx));
	printf("'\n");
	return 2;
}

static int simple_instruction(const char* name)
{
	printf("%s\n", name);
	return 1;
}

int disassemble_instruction(const Chunk* chunk, intptr_t offset)
{
	#define CASE_SIMPLE(OP_CODE) \
		case OP_CODE: return simple_instruction(#OP_CODE)
	#define CASE_CONSTANT(OP_CODE) \
		case OP_CODE: return constant_instruction(#OP_CODE, chunk, offset)

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
		CASE_CONSTANT(OP_GET_GLOBAL);
		CASE_CONSTANT(OP_DEFINE_GLOBAL);
		CASE_CONSTANT(OP_SET_GLOBAL);
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
		CASE_SIMPLE(OP_RETURN);
		default: printf("Unknown opcode %d\n", instruction); return 1;
	}

	#undef CASE_SIMPLE
}

void disassemble_chunk(const Chunk* chunk, const char* name)
{
	printf("== %s ==\n", name);
	for (intptr_t offset = 0; offset < chunk_size(chunk);)
		offset += disassemble_instruction(chunk, offset);
}
