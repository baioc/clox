#include "debug.h"

#include <stdio.h>

#include "chunk.h" // chunk_t
#include "common.h" // intptr_t, uint8_t


static intptr_t simple_instruction(const char* name, intptr_t offset)
{
	printf("%s\n", name);
	return 1;
}

// Prints an instruction OFFSET bytes into CHUNK and returns its size.
static intptr_t disassemble_instruction(const chunk_t* chunk, intptr_t offset)
{
	// print offset
	printf("%04d ", offset);

	// switch on instruction print
	const uint8_t instruction = chunk_get(chunk, offset);
	switch (instruction) {
		case OP_RETURN:
			return simple_instruction("OP_RETURN", offset);
		default:
			printf("Unknown opcode %d\n", instruction);
			return 1;
	}
}

void disassemble_chunk(const chunk_t* chunk, const char* name)
{
	printf("== %s ==\n", name);
	for (intptr_t offset = 0; offset < chunk_size(chunk);)
		offset += disassemble_instruction(chunk, offset);
}
