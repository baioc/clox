#ifndef SGL_LIST_H
#define SGL_LIST_H

#include "core.h"

typedef struct {
	index_t        length;
	index_t        capacity;
	byte_t*        data;
	size_t         elem_size;
	allocator_fn_t allocator;
} list_t;

/** Initializes LIST with initial capacity for N elements of TYPE_SIZE bytes
 * and sets it to use ALLOC (will be set to a default when NULL) as allocator.
 *
 * Returns 0 on success or ENOMEM in case ALLOC fails.
 *
 * list_destroy() must be called on LIST afterwards. */
err_t list_init(list_t* list, index_t n, size_t type_size, allocator_fn_t alloc);

// Frees any resources allocated by list_init().
void list_destroy(list_t* list);

// Gets the number of elements currently stored inside LIST.
index_t list_size(const list_t* list);

// Checks if LIST is empty.
inline bool list_empty(const list_t* list)
{
	return list_size(list) <= 0;
}

// Returns a pointer to the element at INDEX in the dynamic array LIST.
void* list_ref(const list_t* list, index_t index);

/** Inserts TYPE_SIZE bytes of ELEMENT at position INDEX of the LIST.
 *
 * Returns 0 on success or ENOMEM in case ALLOC fails. */
err_t list_insert(list_t* list, index_t index, const void* element);

// Equivalent to list_insert() at the end of the list.
err_t list_append(list_t* list, const void* element);

// Pops the element at INDEX from LIST and copies it to SINK.
void list_remove(list_t* list, index_t index, void* sink);

// Equivalent to list_remove() from the end of the list.
inline void list_pop(list_t* list, void* sink)
{
	list_remove(list, list_size(list) - 1, sink);
}

// Calls FUNC on each pair (index, element) of LIST with extra arg FORWARD.
inline void list_for_each(list_t* list, void (*func)(index_t, void*, void*),
                          void* forward)
{
	const index_t length = list_size(list);
	for (index_t i = 0; i < length; ++i)
		func(i, list_ref(list, i), forward);
}

// Swaps elements in indexes A and B of LIST.
void list_swap(list_t* list, index_t a, index_t b);

/** Searches a SORTED_LIST for ELEMENT using COMPARE to compare its elements.
 *
 * Returns the index of ELEMENT, or a negative value when not found. */
index_t list_search(const list_t* sorted_list, const void* element,
                    compare_fn_t compare);

/** Inserts ELEMENT in SORTED_LIST, using COMPARE to sort its position.
 *
 * Returns the index where it was inserted, or a negative value when
 * list_insert() would have failed. */
index_t list_insert_sorted(list_t* sorted_list, const void* element,
                           compare_fn_t compare);

// Sorts LIST in-place using COMPARE to order its elements.
void list_sort(list_t* list, int (*compare)(const void*, const void*));

// Checks if LIST is sorted (ascending order) through the COMPARE function.
inline bool list_sorted(const list_t* list, compare_fn_t compare)
{
	const index_t length = list_size(list);
	for (index_t i = 1; i < length; ++i) {
		if (compare(list_ref(list, i - 1), list_ref(list, i)) > 0)
			return false;
	}
	return true;
}

#endif // SGL_LIST_H
