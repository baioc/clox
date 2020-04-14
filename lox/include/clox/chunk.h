#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include <assert.h>

#include <sgl/list.h>

#include "common.h" // uint8_t, intptr_t, int8_t
#include "value.h" // ValueArray, Value


// must fit into an uint8_t
typedef enum {
	OP_CONSTANT,
	OP_RETURN,
} opcode;

typedef struct {
	list_t code;
	ValueArray constants;
	list_t lines;
} Chunk;


// Initializes an empty CHUNK. chunk_destroy() must be called on it later.
inline void chunk_init(Chunk* chunk)
{
	list_init(&chunk->code, 0, sizeof(uint8_t));
	value_array_init(&chunk->constants);
	list_init(&chunk->lines, 0, sizeof(int));
}

// Deallocates any resources acquired by chunk_init() on CHUNK.
inline void chunk_destroy(Chunk* chunk)
{
	list_destroy(&chunk->code);
	value_array_destroy(&chunk->constants);
	list_destroy(&chunk->lines);
}

// Gets the current size of CHUNK.
inline long chunk_size(const Chunk* chunk) { return list_size(&chunk->code); }

// Appends BYTE to CHUNK.
inline void chunk_write(Chunk* chunk, uint8_t byte, int line)
{
	list_append(&chunk->code, &byte);
	list_append(&chunk->lines, &line);
}

// Gets a byte from OFFSET inside the CHUNK.
inline uint8_t chunk_get_byte(const Chunk* chunk, intptr_t offset)
{
	assert(0 <= offset);
	assert(offset < chunk_size(chunk));
	return *((uint8_t*)list_ref(&chunk->code, offset));
}

// Gets the line the originated the bytecode at OFFSET inside the CHUNK.
inline int chunk_get_line(const Chunk* chunk, intptr_t offset)
{
	return *((int*)list_ref(&chunk->lines, offset));
}

/** Adds a constant VALUE to the CHUNK's constant pool.
 * Returns the pool index where it was added for later access. */
inline int8_t chunk_add_constant(Chunk* chunk, Value value)
{
	const int index = value_array_size(&chunk->constants);
	assert(index < 255);
	value_array_write(&chunk->constants, value);
	return index;
}

// Gets the constant value added to INDEX of the CHUNK's constant pool.
inline Value chunk_get_constant(const Chunk* chunk, int8_t index)
{
	return value_array_get(&chunk->constants, index);
}

#endif // CLOX_CHUNK_H
