#include <sgl/core.h>

#include <assert.h>
#include <string.h> // strcmp
#include <stdlib.h> // malloc, free


static void swap_primitives(void)
{
	int x = 5;
	int y = 7;
	swap(&x, &y, sizeof(int));
	assert(x == 7);
	assert(y == 5);
}

static void swap_pointers(void)
{
	char* s1 = "Hello, generic";
	char* s2 = "World!";
	swap(&s1, &s2, sizeof(char*));
	assert(strcmp(s1, "World!") == 0);
	assert(strcmp(s2, "Hello, generic") == 0);
}

static void test_freeref(void)
{
	void* ptr = malloc(42);
	assert(ptr != NULL);
	freeref(&ptr);
	assert(ptr == NULL);
	free(ptr); // safe
}

static void test_array_size(void)
{
	int array[50];
	assert(ARRAY_SIZE(array) == 50);
}

err_t main(int argc, const char* argv[])
{
	swap_primitives();
	swap_pointers();
	test_freeref();
	test_array_size();
	return 0;
}
