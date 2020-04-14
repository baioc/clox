#include "chunk.h"

extern inline void chunk_init(Chunk* chunk);
extern inline void chunk_destroy(Chunk* chunk);
extern inline long chunk_size(const Chunk* chunk);
extern inline void chunk_write(Chunk* chunk, uint8_t byte, int line);
extern inline uint8_t chunk_get_byte(const Chunk* chunk, intptr_t offset);
extern inline int chunk_get_line(const Chunk* chunk, intptr_t offset);
extern inline int8_t chunk_add_constant(Chunk* chunk, Value value);
extern inline Value chunk_get_constant(const Chunk* chunk, int8_t index);
