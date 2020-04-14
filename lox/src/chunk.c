#include "chunk.h"

extern inline void chunk_init(chunk_t* chunk);
extern inline void chunk_destroy(chunk_t* chunk);
extern inline long chunk_size(const chunk_t* chunk);
extern inline uint8_t chunk_get(const chunk_t* chunk, intptr_t offset);
extern inline void chunk_write(chunk_t* chunk, uint8_t byte);
