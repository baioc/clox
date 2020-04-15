#ifndef H_SGL_LIST
#define H_SGL_LIST

#include "core.h" // size_t, err_t, byte_t

typedef struct list {
	long    length;
	long    capacity;
	byte_t* data;
	size_t  elem_size;
} list_t;

/** Initializes LIST with capacity for CAPACITY elements of TYPE_SIZE bytes.
 * Returns 0 on success and ENOMEM when out of memory.
 * list_destroy() must be called on LIST afterwards. */
err_t list_init(list_t* list, long capacity, size_t type_size);

// Frees any resources allocated by list_init().
void list_destroy(list_t* list);

// Gets the number of elements currently stored inside LIST.
long list_size(const list_t* list);

// Checks if LIST is empty.
inline bool list_empty(const list_t* list)
{
	return list_size(list) <= 0;
}

// Returns a pointer to the element at INDEX in the dynamic array LIST.
void* list_ref(const list_t* list, long index);

/** Inserts TYPE_SIZE bytes of ELEMENT at position INDEX of the LIST.
 * Returns 0 on success and ENOMEM when out of memory. */
err_t list_insert(list_t* list, long index, const void* element);

// Equivalent to list_insert() at the end of the list.
err_t list_append(list_t* list, const void* element);

// Pops the element at INDEX from LIST and copies its TYPE_SIZE bytes to SINK.
void list_remove(list_t* list, long index, void* sink);

// Equivalent to list_remove() from the end of the list.
inline void list_pop(list_t* list, void* sink)
{
	list_remove(list, list_size(list) - 1, sink);
}

// Calls FUNCTION on each element of LIST.
inline void list_for_each(const list_t* list, void (*function)(void*))
{
	for (long i = 0; i < list_size(list); ++i)
		function(list_ref(list, i));
}

/** Searches an ordered LIST for ELEMENT using COMPARE to compare its elements.
 * Returns the index of ELEMENT, or a negative value when not found. */
long list_bsearch(const list_t* list,
                  const void* element,
                  int (*compare)(const void*, const void*));

#endif // H_SGL_LIST
