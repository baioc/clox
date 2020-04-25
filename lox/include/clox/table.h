#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include <sgl/map.h>

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

// Returns the value identified by the given parameters.
ObjString* table_find_string(const Table* table,
                             const char* str, size_t length, hash_t hash);

// Adds the (KEY -> VALUE) pair to TABLE. Returns true on success.
bool table_put(Table* table, const ObjString* key, Value value);

// Deletes the entry associated with KEY from the TABLE. Returns on success.
bool table_delete(Table* table, const ObjString* key);

#endif // CLOX_TABLE_H
