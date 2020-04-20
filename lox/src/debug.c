#include "debug.h"

#include <stdio.h>

#include "common.h" // uint8_t
#include "value.h" // Value, value_print


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
	// print byte address and line number
	printf("%04d ", offset);
	const int line = chunk_get_line(chunk, offset);
	if (offset > 0 && line == chunk_get_line(chunk, offset - 1))
		printf("   | ");
	else
		printf("%4d ", line);

	// switch on instruction print
	const uint8_t instruction = chunk_get_byte(chunk, offset);
	switch (instruction) {
		case OP_CONSTANT:
			return constant_instruction("OP_CONSTANT", chunk, offset);
		case OP_ADD:
			return simple_instruction("OP_ADD");
		case OP_SUBTRACT:
			return simple_instruction("OP_SUBTRACT");
		case OP_MULTIPLY:
			return simple_instruction("OP_MULTIPLY");
		case OP_DIVIDE:
			return simple_instruction("OP_DIVIDE");
		case OP_NEGATE:
			return simple_instruction("OP_NEGATE");
		case OP_RETURN:
			return simple_instruction("OP_RETURN");
		default:
			printf("Unknown opcode %d\n", instruction);
			return 1;
	}
}

void disassemble_chunk(const Chunk* chunk, const char* name)
{
	printf("== %s ==\n", name);
	for (intptr_t offset = 0; offset < chunk_size(chunk);)
		offset += disassemble_instruction(chunk, offset);
}
