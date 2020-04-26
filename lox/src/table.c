#include "table.h"

#include <string.h> // memcmp

#include <sgl/map.h>
#include <sgl/core.h> // err_t

#include "object.h" // ObjString
#include "common.h" // NULL


struct key {
	const char* str;
	size_t      length;
	hash_t      hash;
};

struct val {
	ObjString* string;
	Value      value;
};

struct adaptor {
	void (*func)(const ObjString*, Value, void*);
	void*  forward;
};


static hash_t hash(const void* ptr, size_t size)
{
	return ((const struct key*)ptr)->hash;
}

static int comp(const void* a, const void* b)
{
	const struct key* s1 = (const struct key*)a;
	const struct key* s2 = (const struct key*)b;
	if (s1->length != s2->length)
		return s1->length - s2->length;
	else if (s1->hash != s2->hash)
		return s1->hash - s2->hash;
	else
		return memcmp(s1->str, s2->str, s1->length);
}

void table_init(Table* tab)
{
	map_init(tab, 0, sizeof(struct key), sizeof(struct val), comp, hash, NULL);
}

void table_destroy(Table* table)
{
	map_destroy(table);
}

bool table_get(const Table* table, const ObjString* k, Value* value)
{
	struct key key = { .str = k->chars, .length = k->length, .hash = k->hash };
	const struct val* found = map_get(table, &key);
	if (found == NULL) return false;
	*value = found->value;
	return true;
}

ObjString* table_find_string(const Table* table, const char* str, size_t length,
                             hash_t hash)
{
	struct key key = { .str = str, .length = length, .hash = hash };
	const struct val* found = map_get(table, &key);
	return found != NULL ? found->string : NULL;
}

extern inline hash_t table_hash(const void* ptr, size_t n);

bool table_put(Table* table, const ObjString* k, Value value)
{
	struct key key = { .str = k->chars, .length = k->length, .hash = k->hash };
	struct val val = { .string = (ObjString*)k, .value = value };
	return map_put(table, &key, &val) < 0;
}

bool table_delete(Table* table, const ObjString* k)
{
	struct key key = { .str = k->chars, .length = k->length, .hash = k->hash };
	return map_delete(table, &key) == 0;
}

static err_t for_each_adaptor(const void* key, void* value, void* forward)
{
	const struct adaptor* adaptor = (struct adaptor*)forward;
	const struct val* entry = (struct val*)value;
	adaptor->func(entry->string, entry->value, adaptor->forward);
	return 0;
}

void table_for_each(const Table* table,
                    void (*func)(const ObjString*, Value, void*),
                    void* forward)
{
	struct adaptor adaptor = { .func = func, .forward = forward };
	map_for_each(table, for_each_adaptor, &adaptor);
}
