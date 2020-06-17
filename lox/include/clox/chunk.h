#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include <sgl/list.h>

#include "common.h" // uint8_t, intptr_t


// Lox VM opcodes, which must must fit into an uint8_t.
enum OpCode {
	OP_CONSTANT, OP_NIL, OP_TRUE, OP_FALSE,
	OP_POP, OP_GET_LOCAL, OP_SET_LOCAL,
	OP_GET_GLOBAL, OP_DEFINE_GLOBAL, OP_SET_GLOBAL,
	OP_GET_UPVALUE, OP_SET_UPVALUE,
	OP_GET_PROPERTY, OP_SET_PROPERTY,
	OP_EQUAL, OP_GREATER, OP_LESS,
	OP_ADD, OP_SUBTRACT, OP_MULTIPLY, OP_DIVIDE,
	OP_NOT, OP_NEGATE,
	OP_PRINT,
	OP_JUMP, OP_JUMP_IF_FALSE, OP_LOOP,
	OP_CALL, OP_CLOSURE, OP_CLOSE_UPVALUE, OP_RETURN,
	OP_CLASS,
	OP_CODE_MAX
};

// A chunk of VM-executable, compiled bytecode.
typedef struct {
	list_t code;
	list_t lines;
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

// Sets a byte from OFFSET inside the CHUNK to VALUE.
void chunk_set_byte(const Chunk* chunk, intptr_t offset, uint8_t value);

// Gets the line that originated the bytecode at OFFSET inside the CHUNK.
int chunk_get_line(const Chunk* chunk, intptr_t offset);

#endif // CLOX_CHUNK_H
