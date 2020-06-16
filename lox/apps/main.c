#include <stdio.h>
#include <stdlib.h> // malloc, free, NULL
#include <sysexits.h>
#include <errno.h>

#include "vm.h"


static int repl(void)
{
	VM vm;
	vm_init(&vm);

	for (char line[1024];;) {
		printf("> ");
		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}
		vm_interpret(&vm, line);
	}

	vm_destroy(&vm);
	return 0;
}

static char* read_file(const char* path)
{
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	const long file_size = ftell(file);
	rewind(file);

	char* buffer = malloc(file_size + 1);
	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}

	const size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
	if (bytes_read < file_size) {
		free(buffer);
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	}
	buffer[bytes_read] = '\0';

	fclose(file);
	return buffer;
}

static int run_file(const char* filename) {
	VM vm;
	Environment* env = set_current_lox_environment(&vm.data);
	vm_init(&vm);
	char* source = read_file(filename);

	const InterpretResult result = vm_interpret(&vm, source);

	free(source);
	vm_destroy(&vm);
	set_current_lox_environment(env);

	if (result == INTERPRET_COMPILE_ERROR) return EX_DATAERR;
	if (result == INTERPRET_RUNTIME_ERROR) return EX_SOFTWARE;
	else return EX_OK;
}

int main(int argc, const char* argv[])
{
	if (argc == 1) {
		return repl();
	} else if (argc == 2) {
		return run_file(argv[1]);
	} else {
		fprintf(stderr, "Usage: clox [path]\n");
		return EX_USAGE;
	}
}
