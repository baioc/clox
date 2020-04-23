#include "list.h"

#include <assert.h>
#include <string.h> // memcpy
#include <stdlib.h> // realloc, bsearch, qsort
#include <errno.h>

#include "core.h" // NULL, swap


err_t list_init(list_t* list, index_t n, size_t type_size, allocator_fn_t alloc)
{
	assert(n >= 0);
	assert(type_size > 0);

	list->length = 0;
	list->capacity = n;
	list->elem_size = type_size;

	list->allocator = alloc != NULL ? alloc : realloc;
	list->data = list->allocator(NULL, type_size * n);
	if (list->data == NULL && n != 0) return ENOMEM;

	return 0;
}

void list_destroy(list_t* list)
{
	list->allocator(list->data, 0);
}

inline index_t list_size(const list_t* list)
{
	return list->length;
}

extern inline bool list_empty(const list_t* list);

inline void* list_ref(const list_t* list, index_t index)
{
	assert(0 <= index);
	assert(index < list->capacity);
	return list->data + list->elem_size * index;
}

static err_t list_grow(list_t* list)
{
	list->capacity = list->capacity > 0 ? list->capacity * 2 : 8;
	void* new = list->allocator(list->data, list->elem_size * list->capacity);
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

inline void list_swap(list_t* list, index_t a, index_t b)
{
	swap(list_ref(list, a), list_ref(list, b), list->elem_size);
}

err_t list_insert(list_t* list, index_t index, const void* element)
{
	assert(0 <= index);
	assert(index <= list->length);

	// append
	const err_t error = list_append(list, element);
	if (error) return error;

	// swap backwards until inserted in the correct position
	for (index_t i = list->length - 1; i > index; --i)
		list_swap(list, i - 1, i);

	return 0;
}

void list_remove(list_t* list, index_t index, void* sink)
{
	assert(0 <= index);
	assert(index < list->length);

	// send copy to output
	byte_t * const source = list_ref(list, index);
	memcpy(sink, source, list->elem_size);
	list->length--;

	// swap "removed" (free space) forward
	for (index_t i = index; i < list->length; ++i)
		list_swap(list, i, i + 1);
}

extern inline void list_pop(list_t* list, void* sink);

typedef void (*for_each_fn)(index_t, void*, void*);
extern inline void list_for_each(list_t* list, for_each_fn func, void* forward);

index_t list_search(const list_t* lst, const void* key, compare_fn_t cmp)
{
	const byte_t* p = bsearch(key, lst->data, lst->length, lst->elem_size, cmp);
	return p == NULL ? -1 : (p - lst->data) / lst->elem_size;
}

index_t list_insert_sorted(list_t* list, const void* src, compare_fn_t compare)
{
	// append
	const err_t error = list_append(list, src);
	if (error) return -1;

	// swap backwards until inserted in the correct position
	index_t i;
	for (i = list->length - 1; i > 0; --i) {
		void * const curr = list_ref(list, i);
		void * const prev = curr - list->elem_size;
		if (compare(prev, curr) < 0) break; // stop when prev < curr
		swap(prev, curr, list->elem_size);
	}
	return i;
}

void list_sort(list_t* list, compare_fn_t compare)
{
	qsort(list->data, list->length, list->elem_size, compare);
}

extern inline bool list_sorted(const list_t* list, compare_fn_t compare);
