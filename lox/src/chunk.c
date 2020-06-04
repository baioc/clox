#include "chunk.h"

#include <limits.h>
#include <assert.h>

#include <sgl/list.h>

#include "value.h"


struct line {
	int number;
	intptr_t address;
};


void chunk_init(Chunk* chunk)
{
	assert(OP_CODE_MAX <= UINT8_MAX);
	list_init(&chunk->code, 0, sizeof(uint8_t), NULL);
	list_init(&chunk->lines, 0, sizeof(struct line), NULL);
	value_array_init(&chunk->constants);
}

void chunk_destroy(Chunk* chunk)
{
	value_array_destroy(&chunk->constants);
	list_destroy(&chunk->lines);
	list_destroy(&chunk->code);
}

long chunk_size(const Chunk* chunk)
{
	return list_size(&chunk->code);
}

void chunk_write(Chunk* chunk, uint8_t byte, int line)
{
	const intptr_t offset = chunk_size(chunk);
	list_append(&chunk->code, &byte);

	// add to lines list only if empty or the last one has a different number
	list_t* lines = &chunk->lines;
	if (list_empty(lines)
	 || ((struct line*)list_ref(lines, list_size(lines) - 1))->number != line
	   ) {
		const struct line line_start = { .number = line, .address = offset };
		list_append(lines, &line_start);
	}
}

uint8_t chunk_get_byte(const Chunk* chunk, intptr_t offset)
{
	return *((uint8_t*)list_ref(&chunk->code, offset));
}

void chunk_set_byte(const Chunk* chunk, intptr_t offset, uint8_t value)
{
	*((uint8_t*)list_ref(&chunk->code, offset)) = value;
}

static int linecmp(const void* a, const void* b)
{
	return ((struct line*)a)->address - ((struct line*)b)->address;
}

int chunk_get_line(const Chunk* chunk, intptr_t offset)
{
	// @NOTE: uses ordered search supposing lines are always sequential
	const list_t* lines = &chunk->lines;
	const struct line key = { .address = offset };
	const long index = list_search(lines, &key, linecmp);
	return index >= 0 ? ((struct line*)list_ref(lines, index))->number : index;
}

int chunk_add_constant(Chunk* chunk, Value value)
{
	const int index = value_array_size(&chunk->constants);
	value_array_write(&chunk->constants, value);
	return index;
}

Value chunk_get_constant(const Chunk* chunk, uint8_t index)
{
	return value_array_get(&chunk->constants, index);
}
