#include "map.h"

#include <assert.h>
#include <string.h> // memcpy
#include <stdlib.h> // realloc
#include <errno.h>

#include "core.h" // byte_t, bool
#include "hash.h" // fnv_1a


// Maximum load factor. Should be tuned based on hash function and usual keys.
#define MAP_MAX_LOAD 0.75

struct map_entry {
	bool in_use;
	bool undead;
	byte_t key[];
};


static void clear_entries(byte_t* entries, byte_t * const values, size_t size)
{
	for (; entries < values; entries += size) {
		struct map_entry* entry = (struct map_entry*)entries;
		entry->in_use = false;
		entry->undead = false;
	}
}

// Finds the nearest power of 2 equal or greater than x.
static unsigned nearest_pow2(int x)
{
	assert(x >= 0);
	if (x < 2) return 1;
	unsigned power = 2;
	x--;
	while (x >>= 1) power <<= 1;
	return power;
}

err_t map_init(map_t* map, index_t n, size_t key_size, size_t value_size,
               compare_fn_t key_cmp, hash_fn_t key_hash, allocator_fn_t alloc)
{
	assert(n >= 0);
	assert(key_size > 0);
	assert(value_size > 0);
	assert(key_cmp != NULL);

	// adjust initial capacity by load factor and round up to nearest power of 2
	n = nearest_pow2(n / MAP_MAX_LOAD);

	map->count = 0;
	map->filled = 0;
	map->capacity = n;
	map->key_size = key_size;
	map->value_size = value_size;
	map->keycmp = key_cmp;
	map->hash = key_hash != NULL ? key_hash : fnv_1a;

	// 1 allocation:   [    N map_entries    |            N values            ]
	//     map->keys:--^       map->values:--^
	map->allocator = alloc != NULL ? alloc : realloc;
	const size_t entry_size = sizeof(struct map_entry) + key_size;
	map->keys = map->allocator(NULL, entry_size * n + value_size * n);
	if (map->keys == NULL && n != 0) return ENOMEM;
	map->values = map->keys + entry_size * n;

	clear_entries(map->keys, map->values, entry_size);
	return 0;
}

void map_destroy(map_t* map)
{
	map->allocator(map->keys, 0);
}

inline index_t map_size(const map_t* map)
{
	return map->count;
}

extern inline bool map_empty(const map_t* map);

static index_t find_entry(const map_t* map, const byte_t* keys, size_t n, const void* key)
{
	// this procedure does not loop infinitely because there will always be
	// at least a few unused buckets due to a maximum load factor less than 1
	assert(MAP_MAX_LOAD < 1);

	const size_t entry_size = sizeof(struct map_entry) + map->key_size;
	index_t tombstone = -1;

	// @NOTE: n should be a power of 2 so we may swap % operations for bitmasks
	assert(nearest_pow2(n) == n);
	n = n - 1;

	// loops over the hashtable's buckets starting from hash(k) % n
	for (index_t k = map->hash(key, map->key_size) & n;; k = (k + 1) & n) {
		struct map_entry* entry = (struct map_entry*)(keys + entry_size * k);
		// probed bucket may be "free", and then could be a re-usable tombstone
		if (!entry->in_use) {
			if (entry->undead && tombstone < 0)
				tombstone = k;
			else if (!entry->undead) // actually free space
				return tombstone >= 0 ? tombstone : k;
		// or it may have the key we were looking for
		} else if (map->keycmp(key, entry->key) == 0) {
			return k;
		}
	}
}

void* map_get(const map_t* map, const void* key)
{
	if (map->count <= 0) return NULL;
	const index_t k = find_entry(map, map->keys, map->capacity, key);
	const size_t entry_size = sizeof(struct map_entry) + map->key_size;
	struct map_entry* entry = (struct map_entry*)(map->keys + entry_size * k);
	return entry->in_use ? map->values + map->value_size * k : NULL;
}

static err_t rehash_table(map_t* map, index_t n)
{
	// initialize and clear a new bucket array with the desired capacity
	const size_t entry_size = sizeof(struct map_entry) + map->key_size;
	byte_t* new_keys = map->allocator(NULL, entry_size*n + map->value_size*n);
	if (new_keys == NULL && n != 0) return ENOMEM;
	byte_t* new_values = new_keys + entry_size * n;
	clear_entries(new_keys, new_values, entry_size);

	// copy every old entry to a newly-computed place in the rehashed table
	map->filled = 0;
	for (index_t i = 0; i < map->capacity; ++i) {
		struct map_entry* src = (struct map_entry*)(map->keys + entry_size * i);
		if (!src->in_use) continue;
		const index_t k = find_entry(map, new_keys, n, src->key);
		struct map_entry* dest = (struct map_entry*)(new_keys + entry_size * k);
		memcpy(dest, src, entry_size);
		byte_t* old_value = map->values + map->value_size * i;
		byte_t* new_value = new_values + map->value_size * k;
		memcpy(new_value, old_value, map->value_size);
		map->filled++;
	}

	// update table's bucket list (remember to free the old one)
	map->capacity = n;
	map->allocator(map->keys, 0);
	map->keys = new_keys;
	map->values = new_values;
	return 0;
}

err_t map_put(map_t* map, const void* key, const void* value)
{
	// check if the table's capacity needs to grow to reduce its load factor
	const index_t capacity = map->capacity;
	if (capacity * MAP_MAX_LOAD < map->filled + 1) {
		const index_t new_capacity = capacity > 0 ? capacity * 2 : 8;
		const err_t error = rehash_table(map, new_capacity);
		if (error) return error;
	}

	// finds entry address; should be done after rehashing (if it happens)
	const index_t k = find_entry(map, map->keys, map->capacity, key);
	const size_t entry_size = sizeof(struct map_entry) + map->key_size;
	struct map_entry* entry = (struct map_entry*)(map->keys + entry_size * k);

	// if entry was vacant, increase hashtable's load
	const bool was_vacant = !entry->in_use;
	if (was_vacant) {
		entry->in_use = true;
		map->count++;
		if (!entry->undead)
			map->filled++;
	}

	// copy key and value pair
	memcpy(entry->key, key, map->key_size);
	memcpy(map->values + map->value_size * k, value, map->value_size);
	return was_vacant ? 0 : -1;
}

err_t map_delete(map_t* map, const void* key)
{
	if (map->count <= 0) return ENOKEY;

	const index_t k = find_entry(map, map->keys, map->capacity, key);
	const size_t entry_size = sizeof(struct map_entry) + map->key_size;
	struct map_entry* entry = (struct map_entry*)(map->keys + entry_size * k);
	if (!entry->in_use) return ENOKEY;

	entry->in_use = false;
	entry->undead = true;
	map->count--;

	return 0;
}

typedef err_t (*for_each_fn_t)(const void*, void*, void*);
err_t map_for_each(const map_t* map, for_each_fn_t func, void* forward)
{
	err_t err = 0;
	const size_t entry_size = sizeof(struct map_entry) + map->key_size;
	for (index_t i = 0; i < map->capacity; ++i) {
		struct map_entry* e = (struct map_entry*)(map->keys + entry_size * i);
		if (!e->in_use) continue;
		err = func(e->key, map->values + map->value_size * i, forward);
		if (err) break;
	}
	return err;
}
