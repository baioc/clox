#ifndef H_SGL_LIST
#define H_SGL_LIST

#include "core.h" // size_t, err_t

typedef struct list {
	long length;
	long capacity;
	byte* data;
	size_t elem_size;
} list_t;

/** Initializes LIST, whose elements have TYPE_SIZE bytes each.
 * Returns 0 on success and ENOMEM when out of memory.
 * list_destroy() must be called on LIST afterwards. */
err_t list_init(list_t* list, size_t type_size);

// Same as list_init(), but LIST has an initial capacity of CAPACITY elements.
err_t list_init_sized(list_t* list, size_t type_size, long capacity);

// Frees any resources allocated by list_init().
void list_destroy(list_t* list);

// Gets the number of elements currently stored inside LIST.
long list_size(const list_t* list);

// Returns a pointer to the element at INDEX in the dynamic array LIST.
void* list_ref(const list_t* list, long index);

/** Inserts TYPE_SIZE bytes of ELEMENT at position INDEX of the LIST.
 * Returns 0 on success and ENOMEM when out of memory. */
err_t list_insert_at(list_t* list, const void* element, long index);

// Equivalent to list_insert_at() the end of the list.
err_t list_insert(list_t* list, const void* element);

// Pops the element at INDEX from LIST and copies its TYPE_SIZE bytes to SINK.
void list_remove_from(list_t* list, void* sink, long index);

// Equivalent to list_remove_from() the end of the list.
void list_remove(list_t* list, void* sink);

// Calls FUNCTION on each element of LIST.
void list_for_each(list_t* list, void (*function)(void*));

#endif // H_SGL_LIST
