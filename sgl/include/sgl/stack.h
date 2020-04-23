#ifndef SGL_STACK_H
#define SGL_STACK_H

#include "core.h" // size_t, index_t, err_t, allocator_fn_t
#include "list.h"

typedef list_t stack_t;

/** Initializes STACK with intial capacity for N elements of TYPE_SIZE bytes
 * and sets it to use ALLOC (will be set to a default when NULL) as allocator.
 *
 * Returns 0 on success or ENOMEM in case ALLOC fails.
 *
 * stack_destroy() must be called on STACK afterwards. */
inline err_t stack_init(stack_t* stack, index_t n, size_t type_size,
                        allocator_fn_t alloc)
{
	return list_init(stack, n, type_size, alloc);
}

// Frees any resources allocated by stack_init().
inline void stack_destroy(stack_t* stack)
{
	list_destroy(stack);
}

// Gets the depth (number of elements inside) of the STACK.
inline index_t stack_size(const stack_t* stack)
{
	return list_size(stack);
}

// Checks if STACK is empty.
inline bool stack_empty(const stack_t* stack)
{
	return list_empty(stack);
}

// Returns a pointer to N-th element below the top of the dynamic STACK.
inline void* stack_peek(const stack_t* stack, index_t n)
{
	return list_ref(stack, stack_size(stack) - 1 - n);
}

/** Pushes TYPE_SIZE bytes of ELEMENT to the top of the STACK.
 *
 * Returns 0 on success or ENOMEM in case ALLOC fails. */
inline err_t stack_push(stack_t* stack, const void* element)
{
	return list_append(stack, element);
}

// Pops the top of the STACK and copies its TYPE_SIZE bytes to SINK.
inline void stack_pop(stack_t* stack, void* sink)
{
	list_pop(stack, sink);
}

#endif // SGL_STACK_H
