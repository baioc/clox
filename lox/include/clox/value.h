#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include <ugly/list.h>

#include "common.h" // bool, uint64_t, NAN_BOXING


// Forward declaration due to cyclic dependencies.
typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef list_t ValueArray;

// Valid Value types.
typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJ,
} ValueType;


#if NAN_BOXING

#define QNAN ((uint64_t)0x7ffc000000000000)
#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define TAG_NIL   1 // 01
#define TAG_FALSE 2 // 10
#define TAG_TRUE  3 // 11
#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))

// Lox values with NaN boxing optimization applied.
typedef uint64_t Value;

union ValueBox {
	Value bits;
	double num;
};

inline Value number_value(double number)
{
	union ValueBox data;
	data.num = number;
	return data.bits;
}

inline double value_as_number(Value value)
{
	union ValueBox data;
	data.bits = value;
	return data.num;
}

inline bool value_is_number(Value value)
{
	return (value & QNAN) != QNAN;
}

inline Value nil_value(void)
{
	return NIL_VAL;
}

inline bool value_is_nil(Value value)
{
	return value == nil_value();
}

inline Value bool_value(bool value)
{
	return value ? TRUE_VAL : FALSE_VAL;
}

inline bool value_as_bool(Value value)
{
	return value == TRUE_VAL;
}

inline bool value_is_bool(Value value)
{
	return value == TRUE_VAL || value == FALSE_VAL;
}

inline Value obj_value(Obj* object)
{
	return (Value)(SIGN_BIT | QNAN | (uint64_t)object);
}

inline Obj* value_as_obj(Value value)
{
	return (Obj*)(value & ~(SIGN_BIT | QNAN));

}

inline bool value_is_obj(Value value)
{
	return (value & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT);
}

#undef FALSE_VAL
#undef TRUE_VAL
#undef NIL_VAL
#undef TAG_TRUE
#undef TAG_FALSE
#undef TAG_NIL
#undef SIGN_BIT
#undef QNAN

#else

// Tagged union for Lox values.
typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
		Obj* obj;
	} as;
} Value;

inline Value number_value(double number)
{
	return (Value){ VAL_NUMBER, { .number = number } };
}

inline double value_as_number(Value value)
{
	return value.as.number;
}

inline bool value_is_number(Value value)
{
	return value.type == VAL_NUMBER;
}

inline Value nil_value(void)
{
	return (Value){ VAL_NIL, { .number = 0 } };
}

inline bool value_is_nil(Value value)
{
	return value.type == VAL_NIL;
}

inline Value bool_value(bool value)
{
	return (Value){ VAL_BOOL, { .boolean = value } };
}

inline bool value_as_bool(Value value)
{
	return value.as.boolean;
}

inline bool value_is_bool(Value value)
{
	return value.type == VAL_BOOL;
}

inline Value obj_value(Obj* object)
{
	return (Value){ VAL_OBJ, { .obj = object } };
}

inline Obj* value_as_obj(Value value)
{
	return value.as.obj;
}

inline bool value_is_obj(Value value)
{
	return value.type == VAL_OBJ;
}

#endif // NAN_BOXING


// Pretty-prints VALUE to stdout.
void value_print(Value value);

// Compare values A and B for equality.
bool value_equal(Value a, Value b);

// Initializes an empty ARRAY. value_array_destroy() must be called on it later.
void value_array_init(ValueArray* array);

// Deallocates any resources acquired by value_array_init() on ARRAY.
void value_array_destroy(ValueArray* array);

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
