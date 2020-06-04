#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include <sgl/map.h>
#include <sgl/hash.h> // hash_t, fnv_1a

#include "value.h" // Value, forward declaration of object.h
#include "common.h" // bool


typedef map_t Table;


// Initializes an empty TABLE. table_destroy() must be called on it later.
void table_init(Table* table);

// Deallocates any resources acquired by table_init().
void table_destroy(Table* table);

/** Get the entry associated with KEY on table and copy its contents to VALUE.
 * Returns true when the entry was found, otherwise false. */
bool table_get(const Table* table, const ObjString* key, Value* value);

// Returns the KEY identified by given parameters, or NULL when not found.
ObjString* table_find_string(const Table* table, const char* str, size_t length,
                             hash_t hash);

// The hashing function internally used in tables.
inline hash_t table_hash(const void* ptr, size_t n)
{
	return fnv_1a(ptr, n);
}

// Adds the (KEY -> VALUE) pair to TABLE. Returns true if key already existed.
bool table_put(Table* table, const ObjString* key, Value value);

// Deletes the entry associated with KEY from the TABLE. Returns on success.
bool table_delete(Table* table, const ObjString* key);

/** Iterates (in unspecified order) through all entries in TABLE, calling FUNC
 * on each one with an extra forwarded argument, eg: FUNC(k, v, FORWARD). */
void table_for_each(const Table* table,
                    void (*func)(const ObjString*, Value*, void*),
                    void* forward);

#endif // CLOX_TABLE_H
