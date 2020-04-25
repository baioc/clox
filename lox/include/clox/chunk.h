#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include <sgl/list.h>

#include "common.h" // uint8_t, intptr_t
#include "value.h" // Value, ValueArray


// Lox VM opcodes, which must must fit into an uint8_t.
enum OpCode {
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,
	OP_NEGATE,
	OP_RETURN,
};

// A chunk of VM-executable, compiled bytecode.
typedef struct {
	list_t     code;
	list_t     lines;
	ValueArray constants;
} Chunk;


// Initializes an empty CHUNK. chunk_destroy() must be called on it later.
void chunk_init(Chunk* chunk);

// Deallocates any resources acquired by chunk_init() for CHUNK.
void chunk_destroy(Chunk* chunk);

// Gets the current size of CHUNK.
long chunk_size(const Chunk* chunk);

// Appends BYTE originated at LINE to CHUNK.
void chunk_write(Chunk* chunk, uint8_t byte, int line);

// Gets a byte from OFFSET inside the CHUNK.
uint8_t chunk_get_byte(const Chunk* chunk, intptr_t offset);

// Gets the line that originated the bytecode at OFFSET inside the CHUNK.
int chunk_get_line(const Chunk* chunk, intptr_t offset);

/** Adds a constant VALUE to the CHUNK's constant pool.
 * Returns the pool index where it was added for later access.
 * @NOTE: Because OP_CONSTANT only uses a single byte for its operand, a
 * constant pool has a maximum capacity of 256 elements.*/
int chunk_add_constant(Chunk* chunk, Value value);

// Gets the constant value added to INDEX of the CHUNK's constant pool.
Value chunk_get_constant(const Chunk* chunk, uint8_t index);

#endif // CLOX_CHUNK_H
