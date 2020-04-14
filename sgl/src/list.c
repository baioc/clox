#include "list.h"

#include <assert.h>
#include <stdlib.h> // malloc, free
#include <errno.h>
#include <string.h> // memcpy, memset

#include "core.h" // size_t, err_t, byte, NULL, swap


inline err_t list_init_sized(list_t* list, size_t type_size, long capacity)
{
	assert(capacity >= 0);
	list->length = 0;
	list->capacity = capacity;
	list->data = malloc(type_size * capacity);
	if (list->data == NULL && capacity != 0) return ENOMEM;
	list->elem_size = type_size;
	return 0;
}

err_t list_init(list_t* list, size_t type_size)
{
	return list_init_sized(list, type_size, 8); // default starting capacity
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

inline void* list_ref(const list_t* list, long index)
{
	assert(0 <= index && index <= list->length);
	return list->data + list->elem_size * index;
}

static inline err_t list_grow(list_t* list)
{
	list->capacity = list->capacity > 0 ? list->capacity * 2 : 8;
	void* new = realloc(list->data, list->elem_size * list->capacity);
	if (new == NULL) return ENOMEM;
	list->data = new;
	return 0;
}

err_t list_insert(list_t* list, const void* element)
{
	// grow current capacity in case its not enough
	if (list->capacity < list->length + 1) {
		const err_t error = list_grow(list);
		if (error) return error;
	}

	// append copy to the end of the list
	byte * const end = list_ref(list, list->length);
	memcpy(end, element, list->elem_size);
	list->length++;

	return 0;
}

err_t list_insert_at(list_t* list, const void* element, long index)
{
	assert(0 <= index && index <= list->length);

	// append
	const err_t error = list_insert(list, element);
	if (error) return error;

	// swap backwards until inserted in the correct position
	for (long i = list->length - 1; i > index; --i) {
		byte * const curr = list_ref(list, i);
		byte * const prev = list_ref(list, i - 1);
		swap(prev, curr, list->elem_size);
	}

	return 0;
}

inline void list_remove_from(list_t* list, void* sink, long index)
{
	assert(0 <= index && index < list->length);

	// send copy to output
	byte * const source = list_ref(list, index);
	memcpy(sink, source, list->elem_size);
	list->length--;

	// swap "removed" (free space) forward
	for (long i = index; i < list->length; ++i) {
		byte * const curr = list_ref(list, i);
		byte * const next = list_ref(list, i + 1);
		swap(curr, next, list->elem_size);
	}
}

void list_remove(list_t* list, void* sink)
{
	return list_remove_from(list, sink, list->length - 1);
}

void list_for_each(list_t* list, void (*function)(void*))
{
	for (long i = 0; i < list->length; ++i)
		function(list_ref(list, i));
}
