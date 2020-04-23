#include <sgl/core.h>

#include <assert.h>
#include <string.h> // strcmp


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

static void static_array_size(void)
{
	int array[50];
	assert(ARRAY_SIZE(array) == 50);
}

static int intcmp(const void* a, const void* b)
{
	return *(int*)a - *(int*)b;
}

static size_t idxlerp(const void* x,
                      const void* in_min, const void* in_max,
                      size_t out_min, size_t out_max)
{
	return out_min + ((*(int*)x - *(int*)in_min) * (out_max - out_min))
	                 / (*(int*)in_max - *(int*)in_min);
}

static void lerp_search(void)
{
	const int array[] = {-6, 0, 2, 3, 6, 7, 11};
	const int n = ARRAY_SIZE(array);

	int key = 6;
	int* found = lerpsearch(&key, array, n, sizeof(int), intcmp, idxlerp);
	assert(found != NULL);
	assert(*found == key);

	key = -11;
	found = lerpsearch(&key, array, n, sizeof(int), intcmp, idxlerp);
	assert(found == NULL);
}

err_t main(int argc, const char* argv[])
{
	swap_primitives();
	swap_pointers();
	static_array_size();
	return 0;
}
