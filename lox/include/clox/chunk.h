#ifndef H_CLOX_CHUNK
#define H_CLOX_CHUNK

#include <assert.h>

#include <sgl/list.h>

#include "common.h" // uint8_t, intptr_t


// must fit into an uint8_t
typedef enum {
	OP_RETURN,
} opcode;

typedef list_t chunk_t;


// Initializes an empty CHUNK. chunk_destroy() must be called on it later.
inline void chunk_init(chunk_t* chunk) { list_init(chunk, 0, sizeof(uint8_t)); }

// Deallocates any resources acquired by chunk_init() on CHUNK.
inline void chunk_destroy(chunk_t* chunk) { list_destroy(chunk); }

// Gets the current size of CHUNK.
inline long chunk_size(const chunk_t* chunk) { return list_size(chunk); }

// Gets a byte from OFFSET inside the CHUNK.
inline uint8_t chunk_get(const chunk_t* chunk, intptr_t offset)
{
	assert(0 <= offset);
	assert(offset < chunk_size(chunk));
	return *((uint8_t*)list_ref(chunk, offset));
}

// Appends BYTE to CHUNK.
inline void chunk_write(chunk_t* chunk, uint8_t byte)
{
	list_append(chunk, &byte);
}

#endif // H_CLOX_CHUNK
