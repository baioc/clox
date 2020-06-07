#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include <sgl/hash.h>

#include "value.h"
#include "common.h" // bool
#include "table.h"
#include "chunk.h"


// Possible Obj types.
typedef enum {
	OBJ_STRING,
	OBJ_FUNCTION,
	OBJ_NATIVE,
} ObjType;

struct Obj {
	ObjType type;
	struct Obj* next;
};

struct ObjString {
	struct Obj obj; // type punning ("struct inheritance") enabler
	hash_t hash;
	size_t length;
	char* chars;
};

typedef struct {
	struct Obj obj;
	ObjString* name;
	int arity;
	Chunk bytecode;
} ObjFunction;

typedef Value (*NativeFn)(int arc, Value argv[]);

typedef struct {
	struct Obj obj;
	NativeFn function;
} ObjNative;


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

inline bool value_is_function(Value value)
{
	return value_obj_is_type(value, OBJ_FUNCTION);
}

inline bool value_is_native(Value value)
{
	return value_obj_is_type(value, OBJ_NATIVE);
}

inline ObjString* value_as_string(Value value)
{
	return (ObjString*)value_as_obj(value);
}

inline char* value_as_c_str(Value value)
{
	return value_as_string(value)->chars;
}

inline ObjFunction* value_as_function(Value value)
{
	return (ObjFunction*)value_as_obj(value);
}

inline NativeFn value_as_native(Value value)
{
	return ((ObjNative*)value_as_obj(value))->function;
}

void obj_print(Value value);

// Deallocates all Objs in the OBJECTS linked list.
void free_objects(Obj** objects);

// Allocates a new ObjString while copying given STR.
ObjString* make_obj_string(Obj** objects, Table* strings,
                           const char* str, size_t str_len);

// Allocates a new ObjString which is the concatenation of PREFIX and SUFFIX.
ObjString* obj_string_concat(Obj** objects, Table* strings,
                             const ObjString* prefix, const ObjString* sufix);

// Allocates a new ObjFunction in the OBJECTS linked list.
ObjFunction* make_obj_function(Obj** objects);

// Allocates a new ObjNative FUNCTION in the OBJECTS linked list.
ObjNative* make_obj_native(Obj** objects, NativeFn function);

#endif // CLOX_OBJECT_H
