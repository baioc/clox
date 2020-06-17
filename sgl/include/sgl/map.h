#ifndef SGL_MAP_H
#define SGL_MAP_H

#include "core.h"
#include "hash.h" // hash_fn_t

// Generic hash table with constant amortized access, insertions and deletes.
typedef struct {
	index_t count;
	index_t filled;
	index_t capacity;
	byte_t* keys;
	byte_t* values;
	size_t key_size;
	size_t value_size;
	compare_fn_t keycmp;
	hash_fn_t hash;
	allocator_fn_t allocator;
} map_t;

/** Initializes MAP with initial capacity for N mappings from keys with KEY_SIZE
 * bytes - that should be comparable by KEY_CMP and hashable by KEY_HASH
 * - to values with VALUE_SIZE bytes.
 *
 * If ALLOC or KEY_HASH are NULL, default implementations will be provided.
 * map_destroy() must be called on MAP afterwards.
 *
 * Returns 0 on success or ENOMEM when ALLOC fails. */
err_t map_init(map_t* map, index_t n, size_t key_size, size_t value_size,
               compare_fn_t key_cmp, hash_fn_t key_hash, allocator_fn_t alloc);
// @XXX: when no key_hash is provided, we could use a tree to implement this...

// Frees any resources allocated by map_init().
void map_destroy(map_t* map);

// Gets the number of used entries in MAP.
index_t map_size(const map_t* map);

// Checks if MAP is empty.
inline bool map_empty(const map_t* map)
{
	return map_size(map) <= 0;
}

/** Returns a pointer to the value associated with KEY in the dynamic MAP, this
 * will be NULL when the pairing does not exist. */
void* map_get(const map_t* map, const void* key);

/** Puts the (KEY -> VALUE) entry on the MAP, overwritting any previous one.
 *
 * Returns ENOMEM in case ALLOC fails, a negative number if an entry with
 * KEY already existed and has its value overwritten; zero otherwise. */
err_t map_put(map_t* map, const void* key, const void* value);

// Deletes KEY's entry from MAP. Returns 0 on success and ENOKEY otherwise.
err_t map_delete(map_t* map, const void* key);

/** Iterates (in unspecified order) through all entries in MAP, calling FUNC on
 * each one with an extra forwarded argument, eg: FUNC(k, v, FORWARD).
 *
 * The iteration will be halted in case FUNC returns a non-zero value, which
 * will be then returned by this function. Returns 0 otherwise. */
err_t map_for_each(const map_t* map, err_t (*func)(const void*, void*, void*),
                   void* forward);

#endif // SGL_MAP_H
