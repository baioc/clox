#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include <assert.h>

#include <sgl/list.h>

#include "common.h" // uint8_t, intptr_t, int8_t
#include "value.h" // Value, ValueArray


// must fit into an uint8_t
enum OpCode {
	OP_CONSTANT,
	OP_RETURN,
};

typedef struct {
	list_t     code;
	ValueArray constants;
	list_t     lines;
} Chunk;


// Initializes an empty CHUNK. chunk_destroy() must be called on it later.
void chunk_init(Chunk* chunk);

// Deallocates any resources acquired by chunk_init() for CHUNK.
inline void chunk_destroy(Chunk* chunk)
{
	list_destroy(&chunk->code);
	value_array_destroy(&chunk->constants);
	list_destroy(&chunk->lines);
}

// Gets the current size of CHUNK.
inline long chunk_size(const Chunk* chunk)
{
	return list_size(&chunk->code);
}

// Appends BYTE originated at LINE to CHUNK.
void chunk_write(Chunk* chunk, uint8_t byte, int line);

// Gets a byte from OFFSET inside the CHUNK.
inline uint8_t chunk_get_byte(const Chunk* chunk, intptr_t offset)
{
	return *((uint8_t*)list_ref(&chunk->code, offset));
}

// Gets the line that originated the bytecode at OFFSET inside the CHUNK.
int chunk_get_line(const Chunk* chunk, intptr_t offset);

/** Adds a constant VALUE to the CHUNK's constant pool.
 * Returns the pool index where it was added for later access.
 * @NOTE: Because OP_CONSTANT only uses a single byte for its operand, a
 * constant pool has a maximum capacity of 256 elements.*/
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
