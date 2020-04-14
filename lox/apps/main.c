#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[])
{
	chunk_t chunk;
	chunk_init(&chunk);

	chunk_write(&chunk, OP_RETURN);
	disassemble_chunk(&chunk, "test chunk");

	chunk_destroy(&chunk);

	return 0;
}
