#include <clox/chunk.h>
#include <clox/vm.h>

int main(int argc, const char* argv[])
{
	VM vm;
	vm_init(&vm);

	Chunk chunk;
	chunk_init(&chunk);

	chunk_write(&chunk, OP_CONSTANT, 123);
	chunk_write(&chunk, chunk_add_constant(&chunk, 1.2), 123);

	chunk_write(&chunk, OP_CONSTANT, 123);
	chunk_write(&chunk, chunk_add_constant(&chunk, 3.4), 123);

	chunk_write(&chunk, OP_ADD, 123);

	chunk_write(&chunk, OP_CONSTANT, 123);
	chunk_write(&chunk, chunk_add_constant(&chunk, 5.6), 123);

	chunk_write(&chunk, OP_DIVIDE, 123);

	chunk_write(&chunk, OP_NEGATE, 123);

	chunk_write(&chunk, OP_RETURN, 123);

	vm_interpret(&vm, &chunk);

	chunk_destroy(&chunk);

	vm_destroy(&vm);

	return 0;
}
