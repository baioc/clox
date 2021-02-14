#include "value.h"

#include <stdio.h>
#include <string.h> // memcmp

#include "object.h"
#include "memory.h" // reallocate


extern inline Value number_value(double number);
extern inline double value_as_number(Value value);
extern inline bool value_is_number(Value value);
extern inline Value nil_value(void);
extern inline bool value_is_nil(Value value);
extern inline Value bool_value(bool value);
extern inline bool value_as_bool(Value value);
extern inline bool value_is_bool(Value value);
extern inline Value obj_value(Obj* object);
extern inline Obj* value_as_obj(Value value);
extern inline bool value_is_obj(Value value);

void value_print(Value value)
{
#if NAN_BOXING
	if (value_is_bool(value)) printf(value_as_bool(value) ? "true" : "false");
	else if (value_is_nil(value)) printf("nil");
	else if (value_is_number(value)) printf("%g", value_as_number(value));
	else if (value_is_obj(value)) obj_print(value);
#else
	switch (value.type) {
		case VAL_BOOL: printf(value_as_bool(value) ? "true" : "false"); break;
		case VAL_NIL: printf("nil"); break;
		case VAL_NUMBER: printf("%g", value_as_number(value)); break;
		case VAL_OBJ: obj_print(value); break;
	}
#endif
}

bool value_equal(Value a, Value b)
{
#if NAN_BOXING
	return value_is_number(a) && value_is_number(b)
	     ? value_as_number(a) == value_as_number(b)
	     : a == b;
#else
	if (a.type != b.type) return false;
	switch (a.type) {
		case VAL_BOOL: return value_as_bool(a) == value_as_bool(b);
		case VAL_NIL: return true;
		case VAL_NUMBER: return value_as_number(a) == value_as_number(b);
		// @NOTE: this works because all strings are interned
		case VAL_OBJ: return value_as_string(a) == value_as_string(b);
	}
#endif
}

static void* value_array_malloc(struct allocator *ctx, void* ptr, size_t size)
{
	return reallocate((Environment*)ctx->environment, ptr, size, "ValueArray");
}

void value_array_init(ValueArray* array, Environment* env)
{
	list_init(array, 0,
	          sizeof(Value),
	          (struct allocator){ .method = value_array_malloc, .environment = env });
}

void value_array_destroy(ValueArray* array)
{
	list_destroy(array);
}

extern inline int value_array_size(const ValueArray* array);
extern inline Value value_array_get(const ValueArray* array, int index);
extern inline void value_array_write(ValueArray* array, Value value);
