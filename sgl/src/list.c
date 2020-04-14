#include "list.h"

#include <assert.h>
#include <stdlib.h> // malloc, free
#include <errno.h>
#include <string.h> // memcpy, memset

#include "core.h" // size_t, err_t, NULL, byte_t, swap


extern inline long list_size(const list_t* list);

extern inline bool list_empty(const list_t* list);

extern inline void* list_ref(const list_t* list, long index);

extern inline void list_pop(list_t* list, void* sink);

extern inline void list_for_each(const list_t* list, void (*function)(void*));


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
	assert(0 <= index && index <= list->length);

	// append
	const err_t error = list_append(list, element);
	if (error) return error;

	// swap backwards until inserted in the correct position
	for (long i = list->length - 1; i > index; --i) {
		byte_t * const curr = list_ref(list, i);
		byte_t * const prev = list_ref(list, i - 1);
		swap(prev, curr, list->elem_size);
	}

	return 0;
}

void list_remove(list_t* list, long index, void* sink)
{
	assert(0 <= index && index < list->length);

	// send copy to output
	byte_t * const source = list_ref(list, index);
	memcpy(sink, source, list->elem_size);
	list->length--;

	// swap "removed" (free space) forward
	for (long i = index; i < list->length; ++i) {
		byte_t * const curr = list_ref(list, i);
		byte_t * const next = list_ref(list, i + 1);
		swap(curr, next, list->elem_size);
	}
}
