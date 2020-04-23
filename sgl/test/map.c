#include <sgl/map.h>

#include <assert.h>
#include <string.h> // strcmp, memcpy
#include <stdlib.h> // rand, malloc
#include <errno.h>

#include <sgl/core.h> // ARRAY_SIZE
#include <sgl/hash.h> // fnv_1a


static int strrefcmp(const void *a, const void *b)
{
	const char *str1 = *(const char **)a;
	const char *str2 = *(const char **)b;
	return strcmp(str1, str2);
}

static hash_t strhash(const void* ptr, size_t bytes)
{
	const char* str = *(const char**)ptr;
	const size_t n = strlen(str);  // bytes is actualy sizeof(char*)
	return fnv_1a(str, n);
}

int main(int argc, const char* argv[])
{
	const char* numbers[] = {"zero", "one", "two", "three", "four", "five"};
	const int n = ARRAY_SIZE(numbers);

	// initialize map: string -> int, should be empty
	map_t dict;
	err_t err = map_init(&dict, 2, sizeof(char*), sizeof(int),
	                     strrefcmp, strhash, NULL);
	assert(!err);
	assert(map_empty(&dict));

	// put entries and check final size
	for (int i = 0; i < n; ++i) {
		err = map_put(&dict, &numbers[i], &i);
		assert(!err);
	}
	assert(map_size(&dict) == n);

	// choose and copy a random key
	const int del = rand() % n;
	const size_t del_size = strlen(numbers[del]) + 1;
	char* deleted = malloc(del_size);
	memcpy(deleted, numbers[del], del_size);
	assert(deleted != NULL);
	assert(strcmp(deleted, numbers[del]) == 0);

	// delete that randomly selected entry, test size and access
	err = map_delete(&dict, &deleted);
	free(deleted);
	assert(!err);
	assert(map_size(&dict) == n - 1);
	assert(map_get(&dict, &numbers[del]) == NULL);

	// for all other entries, compare with original array
	for (int i = 0; i < n; ++i) {
		if (i == del) continue;
		int* numeral = map_get(&dict, &numbers[i]);
		assert(numeral != NULL);
		assert(*numeral == i);
	}

	// try deleting the same entry
	err = map_delete(&dict, &numbers[del]);
	assert(err == ENOKEY);

	// deallocate map
	map_destroy(&dict);
	return 0;
}
