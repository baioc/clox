#include <assert.h>

#include <clox/chunk.h>
#include <clox/debug.h>


int main(int argc, const char* argv[])
{
	Chunk chunk;
	chunk_init(&chunk);

	chunk_write(&chunk, OP_CONSTANT, 123);
	chunk_write(&chunk, chunk_add_constant(&chunk, 1.2), 123);

	chunk_write(&chunk, OP_RETURN, 123);

	disassemble_chunk(&chunk, "test chunk");
	assert(chunk_size(&chunk) == 3);

	chunk_destroy(&chunk);

	return 0;
}
