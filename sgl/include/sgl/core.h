#ifndef SGL_CORE_H
#define SGL_CORE_H

#include <stddef.h> // size_t, NULL, offsetof
#include <stdbool.h> // bool

// Given that C is a byte-manipulation language, this should be standard.
typedef unsigned char byte_t;

// A type used for error codes, should be zero when there were no errors.
typedef int err_t;

// Signed type used to index data structures.
typedef long index_t;

/** One-function memory allocator which must support the following protocol:
 * - PTR == NULL and SIZE != 0 -> returns a new allocation of SIZE bytes.
 * - PTR != NULL and SIZE == 0 -> frees the allocated contents of PTR.
 * - PTR != NULL and SIZE != 0 -> reallocates SIZE bytes and frees PTR.
 * - PTR == NULL and SIZE == 0 -> does nothing.
 *
 * When performing allocations, this function returns a pointer to the
 * allocated memory block when it was sucessfull or NULL if it fails.
 * Return is undefined for all other cases.
 *
 * The third option should be used when the memory allocating system supports
 * in-place resizing of previously allocated blocks; it may also decide to use
 * another location for the resized block, in which case the previous one is
 * freed. When this reallocation fails, the old block should be preserved.
 *
 * A function in the standard library supporting this protocol is realloc(). */
typedef void* (*allocator_fn_t)(void* ptr, size_t size);

/** Generic comparison function for the contents pointed to by its arguments:
 *
 * - It returns a number ==0  when A == B.
 * - It returns a number < 0 when A < B.
 * - It returns a number <=0 when A <= B.
 * - It returns a number > 0 when A > B.
 * - It returns a number >=0 when A >= B.
 *
 * For instance, if A and B are pointers to int, one could use (*a - *b) as
 * the return of one such function. */
typedef int (*compare_fn_t)(const void* a, const void* b);

// Gets number of elements of static array ARR.
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Given address PTR of field MEMBER, finds address of its CONTAINER struct.
#define CONTAINER_OF(ptr, container, member) \
	((container*)((byte_t*)(ptr) - offsetof(container, member)))

// Swaps the memory contents of the first SIZE bytes pointed to by A and B.
void swap(void *a, void *b, size_t size);

/** Searches an ORDERED_ARRAY of N items, each with SIZE bytes, for KEY,
 * where COMPAR is used to compare its elements and LERP to interpolate from
 * a target value to its expected index in an ordered array.
 *
 * Returns a pointer to the element KEY or NULL when it could not be found.
 */
void* lerpsearch(const void* key, const void* ordered_array, size_t n, size_t size, compare_fn_t,
                 size_t (*lerp)(const void*, const void*, const void*, size_t, size_t));

#endif // SGL_CORE_H
