#ifndef SGL_HASH_H
#define SGL_HASH_H

#include "core.h" // size_t

// Return type for hash functions.
typedef unsigned long hash_t;

// Generic function to hash N bytes from what is pointed to by PTR.
typedef hash_t (*hash_fn_t)(const void* ptr, size_t n);

// FNV-1a hashing algorithm: http://www.isthe.com/chongo/tech/comp/fnv/
hash_t fnv_1a(const void* ptr, size_t n);

#endif // SGL_HASH_H
