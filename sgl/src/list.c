#include "list.h"

#include <assert.h>
#include <stdlib.h> // malloc, free, bsearch
#include <errno.h>
#include <string.h> // memcpy, memset

#include "core.h" // NULL, swap


err_t list_init(list_t* list, long capacity, size_t type_size)
{
	assert(capacity >= 0);
	list->length = 0;
	list->capacity = capacity;
	list->data = malloc(type_size * capacity);
	if (list->data == NULL && capacity != 0) return ENOMEM;
	list->elem_size = type_size;
	return 0;
}

void list_destroy(list_t* list)
{
	free(list->data);
	memset(list, 0, sizeof(list_t));
}

long list_size(const list_t* list)
{
	return list->length;
}

extern inline bool list_empty(const list_t* list);

inline void* list_ref(const list_t* list, long index)
{
	assert(0 <= index);
	assert(index < list->capacity);
	return list->data + list->elem_size * index;
}

static err_t list_grow(list_t* list)
{
	list->capacity = list->capacity > 0 ? list->capacity * 2 : 8;
	void* new = realloc(list->data, list->elem_size * list->capacity);
	if (new == NULL) return ENOMEM;
	list->data = new;
	return 0;
}

err_t list_append(list_t* list, const void* element)
{
	// grow current capacity in case its not enough
	if (list->capacity < list->length + 1) {
		const err_t error = list_grow(list);
		if (error) return error;
	}

	// append copy to the end of the list
	byte_t * const end = list_ref(list, list->length);
	memcpy(end, element, list->elem_size);
	list->length++;

	return 0;
}

err_t list_insert(list_t* list, long index, const void* element)
{
	assert(0 <= index);
	assert(index <= list->length);

	// append
	const err_t error = list_append(list, element);
	if (error) return error;

	// swap backwards until inserted in the correct position
	for (long i = list->length - 1; i > index; --i)
		swap(list_ref(list, i - 1), list_ref(list, i), list->elem_size);

	return 0;
}

void list_remove(list_t* list, long index, void* sink)
{
	// send copy to output
	byte_t * const source = list_ref(list, index);
	memcpy(sink, source, list->elem_size);
	list->length--;

	// swap "removed" (free space) forward
	for (long i = index; i < list->length; ++i)
		swap(list_ref(list, i), list_ref(list, i + 1), list->elem_size);
}

extern inline void list_pop(list_t* list, void* sink);

extern inline void list_for_each(const list_t* list, void (*function)(void*));

typedef int (*compare_fn)(const void*, const void*);
long list_bsearch(const list_t* lst, const void* key, compare_fn cmp)
{
	const byte_t* p = bsearch(key, lst->data, lst->length, lst->elem_size, cmp);
	return p == NULL ? -1 : (p - lst->data) / lst->elem_size;
}
