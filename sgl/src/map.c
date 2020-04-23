#include "map.h"

#include <assert.h>
#include <string.h> // memcpy
#include <errno.h>

#include "core.h" // byte_t, bool
#include "hash.h" // fnv_1a


// Maximum load factor. Should be tuned based on hash function and usual keys.
#define MAP_MAX_LOAD 0.75

struct map_entry {
	bool   in_use;
	bool   undead;
	byte_t key_value[];
};


static void clear_entry(index_t k, void* entry, void* unused)
{
	((struct map_entry*)entry)->in_use = false;
	((struct map_entry*)entry)->undead = false;
}

err_t map_init(map_t* map,
               index_t n,
               size_t key_size,
               size_t value_size,
               compare_fn_t key_cmp,
               hash_fn_t key_hash,
               allocator_fn_t alloc)
{
	assert(n >= 0);
	assert(key_size > 0);
	assert(value_size >= 0);
	assert(key_cmp != NULL);

	n /= MAP_MAX_LOAD; // adjust capacity by load factor
	const size_t entry_size = sizeof(struct map_entry) + key_size + value_size;
	const err_t err = list_init(&map->buckets, n, entry_size, alloc);
	if (err) return err;
	list_for_each(&map->buckets, clear_entry, NULL);

	map->count = 0;
	map->key_size = key_size;
	map->keycmp = key_cmp;
	map->hash = key_hash != NULL ? key_hash : fnv_1a;

	return 0;
}

void map_destroy(map_t* map)
{
	list_destroy(&map->buckets);
}

inline index_t map_size(const map_t* map)
{
	return map->count;
}

extern inline bool map_empty(const map_t* map);

static struct map_entry* find_entry(const map_t* map,
                                    const list_t* buckets,
                                    const void* key)
{
	// this procedure does not loop infinitely because there will always be
	// at least a few unused buckets due to a maximum load factor less than 1
	assert(MAP_MAX_LOAD < 1);

	const index_t n = buckets->capacity;
	struct map_entry* tombstone = NULL;

	// loops over the hashtable's buckets starting from the (hash(k) % n
	for (index_t k = map->hash(key, map->key_size) % n; ; k = (k + 1) % n) {
		struct map_entry* entry = list_ref(buckets, k);
		// probed bucket may be "free", and then could be a re-usable tombstone
		if (!entry->in_use) {
			if (entry->undead && tombstone == NULL)
				tombstone = entry;
			else if (!entry->undead) // actually free space
				return tombstone == NULL ? entry : tombstone;
		} // or it may have the key we were looking for
		else if (map->keycmp(key, entry->key_value) == 0) {
			return entry;
		}
	}
}

void* map_get(const map_t* map, const void* key)
{
	if (map_empty(map)) return NULL;
	struct map_entry* entry = find_entry(map, &map->buckets, key);
	return entry->in_use ? entry->key_value + map->key_size : NULL;
}

static err_t rehash_table(map_t* map, index_t capacity)
{
	// initialize a new bucket list with the desired increased capacity
	list_t new_buckets;
	const err_t err = list_init(&new_buckets, capacity,
	                            map->buckets.elem_size, map->buckets.allocator);
	if (err) return err;
	list_for_each(&new_buckets, clear_entry, NULL);

	// copy every old entry to a newly-computed place in the rehashed table
	byte_t* current = map->buckets.data;
	for (index_t i = 0; i < map->buckets.capacity; ++i) {
		struct map_entry* src = (struct map_entry*)current;
		current += map->buckets.elem_size;
		if (!src->in_use) continue;
		struct map_entry* dest = find_entry(map, &new_buckets, src->key_value);
		memcpy(dest, src, map->buckets.elem_size);
		new_buckets.length++;
	}

	// update table's bucket list
	list_destroy(&map->buckets);
	map->buckets = new_buckets;
	return 0;
}

err_t map_put(map_t* map, const void* key, const void* value)
{
	// check if the table's capacity needs to grow to reduce its load factor
	const index_t capacity = map->buckets.capacity;
	if (capacity * MAP_MAX_LOAD < map->buckets.length + 1) {
		const index_t new_capacity = capacity > 0 ? capacity * 2 : 8;
		const err_t error = rehash_table(map, new_capacity);
		if (error) return error;
	}

	// finds entry address, should be done after rehashing (if it happens)
	struct map_entry* entry = find_entry(map, &map->buckets, key);

	// if entry was totally vacant, increase hashtable's load
	if (!entry->in_use) {
		entry->in_use = true;
		map->count++;
		if (!entry->undead)
			map->buckets.length++;
	}

	// copy key and value pair to entry
	memcpy(entry->key_value, key, map->key_size);
	const size_t value_size = map->buckets.elem_size
	                        - sizeof(struct map_entry) - map->key_size;
	memcpy(entry->key_value + map->key_size, value, value_size);
	return 0;
}

err_t map_delete(map_t* map, const void* key)
{
	if (map_empty(map)) return ENOKEY;

	struct map_entry* entry = find_entry(map, &map->buckets, key);
	if (!entry->in_use) return ENOKEY;

	entry->in_use = false;
	entry->undead = true;
	map->count--;

	return 0;
}
