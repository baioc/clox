#include "chunk.h"

#include <assert.h>
#include <stdlib.h> // bsearch


struct line {
	int      number;
	intptr_t address;
};


void chunk_init(Chunk* chunk)
{
	list_init(&chunk->code, 0, sizeof(uint8_t));
	value_array_init(&chunk->constants);
	list_init(&chunk->lines, 0, sizeof(struct line));
}

void chunk_destroy(Chunk* chunk)
{
	list_destroy(&chunk->code);
	value_array_destroy(&chunk->constants);
	list_destroy(&chunk->lines);
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

static int linecmp(const void* a, const void* b)
{
	return ((struct line*)a)->address - ((struct line*)b)->address;
}

int chunk_get_line(const Chunk* chunk, intptr_t offset)
{
	// @NOTE: uses bsearch supposing lines are always written sequentially
	const list_t* lines = &chunk->lines;
	const struct line key = { .address = offset };
	const long index = list_bsearch(lines, &key, linecmp);
	return index >= 0 ? ((struct line*)list_ref(lines, index))->number : index;
}

int8_t chunk_add_constant(Chunk* chunk, Value value)
{
	const int index = value_array_size(&chunk->constants);
	assert(index < 255);
	value_array_write(&chunk->constants, value);
	return index;
}

Value chunk_get_constant(const Chunk* chunk, int8_t index)
{
	return value_array_get(&chunk->constants, index);
}
