#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include <sgl/list.h>

#include "common.h" // bool"


// Forward declaration due to cyclic dependencies.
typedef struct Obj Obj;
typedef struct ObjString ObjString;

// Valid Value types.
typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJ,
} ValueType;

// Tagged union for Lox values.
typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
		Obj* obj;
	} as;
} Value;

typedef list_t ValueArray;


inline Value bool_value(bool value)
{
	return (Value){ VAL_BOOL, { .boolean = value } };
}

inline Value nil_value(void)
{
	return (Value){ VAL_NIL, { .number = 0 } };
}

inline Value number_value(double number)
{
	return (Value){ VAL_NUMBER, { .number = number } };
}

inline Value obj_value(Obj* object)
{
	return (Value){ VAL_OBJ, { .obj = object } };
}

inline bool value_is_bool(Value value)
{
	return value.type == VAL_BOOL;
}

inline bool value_is_nil(Value value)
{
	return value.type == VAL_NIL;
}

inline bool value_is_number(Value value)
{
	return value.type == VAL_NUMBER;
}

inline bool value_is_obj(Value value)
{
	return value.type == VAL_OBJ;
}

inline bool value_as_bool(Value value)
{
	return value.as.boolean;
}

inline double value_as_number(Value value)
{
	return value.as.number;
}

inline Obj* value_as_obj(Value value)
{
	return value.as.obj;
}

void value_print(Value value);

bool value_equal(Value a, Value b);

// Initializes an empty ARRAY. value_array_destroy() must be called on it later.
inline void value_array_init(ValueArray* array)
{
	list_init(array, 0, sizeof(Value), NULL);
}

// Deallocates any resources acquired by value_array_init() on ARRAY.
inline void value_array_destroy(ValueArray* array)
{
	list_destroy(array);
}

// Gets the current size of ARRAY.
inline int value_array_size(const ValueArray* array)
{
	return list_size(array);
}

// Gets a value from position INDEX of ARRAY.
inline Value value_array_get(const ValueArray* array, int index)
{
	return *((Value*)list_ref(array, index));
}

// Adds VALUE to ARRAY.
inline void value_array_write(ValueArray* array, Value value)
{
	list_append(array, &value);
}

#endif // CLOX_VALUE_H
