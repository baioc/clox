#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "value.h"
#include "common.h" // bool
#include "vm.h"


typedef enum {
	OBJ_STRING,
} ObjType;

struct Obj {
	ObjType type;
	struct  Obj* next;
};

struct ObjString {
	struct Obj obj; // type punning ("struct inheritance") enabler
	int    length;
	char   chars[]; // flexible array
};


inline ObjType obj_type(Value value)
{
	return value_as_obj(value)->type;
}

inline bool value_obj_is_type(Value value, ObjType type)
{
	return value_is_obj(value) && value_as_obj(value)->type == type;
}

inline bool value_is_string(Value value)
{
	return value_obj_is_type(value, OBJ_STRING);
}

inline ObjString* value_as_string(Value value)
{
	return (ObjString*)value_as_obj(value);
}

inline char* value_as_c_str(Value value)
{
	return value_as_string(value)->chars;
}

void obj_print(Value value);

ObjString* make_obj_string(Obj** objects, int length);

ObjString* make_obj_string_copy(Obj** objects, const char* c_str, int length);

#endif // CLOX_OBJECT_H
