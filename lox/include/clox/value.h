#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include <stdio.h>

#include <sgl/list.h>

#include "common.h" // uint8_t


typedef double Value;

typedef list_t ValueArray;


inline void value_print(Value value)
{
	printf("%g", value);
}

// Initializes an empty ARRAY. value_array_destroy() must be called on it later.
inline void value_array_init(ValueArray* array)
{
	list_init(array, 0, sizeof(Value));
}

// Deallocates any resources acquired by value_array_init() on ARRAY.
inline void value_array_destroy(ValueArray* array) { list_destroy(array); }

// Gets the current size of ARRAY.
inline int value_array_size(const ValueArray* array) {return list_size(array);}

// Gets a value from position INDEX of ARRAY.
inline Value value_array_get(const ValueArray* array, int index)
{
	assert(0 <= index);
	assert(index < value_array_size(array));
	return *((Value*)list_ref(array, index));
}

// Adds VALUE to ARRAY.
inline void value_array_write(ValueArray* array, Value value)
{
	list_append(array, &value);
}

#endif // CLOX_VALUE_H
