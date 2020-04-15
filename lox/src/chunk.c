#include "chunk.h"

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

extern inline void chunk_destroy(Chunk* chunk);

extern inline long chunk_size(const Chunk* chunk);

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

extern inline uint8_t chunk_get_byte(const Chunk* chunk, intptr_t offset);

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

extern inline int8_t chunk_add_constant(Chunk* chunk, Value value);

extern inline Value chunk_get_constant(const Chunk* chunk, int8_t index);
