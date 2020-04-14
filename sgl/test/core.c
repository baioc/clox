#include <sgl/core.h>

#include <assert.h>
#include <string.h> // strcmp


static inline void swap_primitives(void)
{
	int x = 5;
	int y = 7;
	swap(&x, &y, sizeof(int));
	assert(x == 7);
	assert(y == 5);
}

static inline void swap_pointers(void)
{
	char* s1 = "Hello, generic";
	char* s2 = "World!";
	swap(&s1, &s2, sizeof(char*));
	assert(strcmp(s1, "World!") == 0);
	assert(strcmp(s2, "Hello, generic") == 0);
}

err_t main(int argc, const char* argv[])
{
	swap_primitives();
	swap_pointers();
	return 0;
}
