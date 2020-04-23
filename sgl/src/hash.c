#include "hash.h"

#include <stdint.h> // uint32_t


hash_t fnv_1a(const void* ptr, size_t n)
{
	uint32_t hash = 2166136261u;
	const byte_t* bytes = ptr;
	for (size_t i = 0; i < n; ++i) {
		hash ^= *bytes++;
		hash *= 16777619;
	}
	return hash;
}
