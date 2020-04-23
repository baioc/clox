#include "core.h"

#include <string.h> // memcpy


void swap(void *a, void *b, size_t size)
{
	byte_t tmp[size];
	memcpy(tmp, a, size);
	memcpy(a, b, size);
	memcpy(b, tmp, size);
}

typedef size_t (*lerp_fn)(const void*, const void*, const void*, size_t, size_t);
void* lerpsearch(const void* key,
                 const void* base,
                 size_t length,
                 size_t elem_size,
                 int (*compare)(const void*, const void*),
                 lerp_fn lerp)
{
	size_t low = 0;
	size_t high = length - 1;
	const void* min = base;
	const void* max = base + elem_size*high;

	while (low <= high) {
		// interpolate key and check bounds on expected index
		const size_t expected = lerp(key, min, max, low, high);
		if (expected < low || high < expected) return NULL;

		// get the "predicted" address and compare it to the searched key
		const void* lerped = base + elem_size*expected;
		const int diff = compare(key, lerped);

		// either found, or at least one of the bounds needs a reduction
		if (diff == 0) {
			return (void*)lerped;
		} else if (diff > 0) {
			low = expected + 1;
			min = lerped + elem_size;
		} else {
			high = expected - 1;
			max = lerped - elem_size;
		}
	}

	return NULL;
}
