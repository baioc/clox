#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include <ugly/hash.h>

#include "value.h"
#include "common.h" // bool
#include "table.h"
#include "chunk.h"


// Possible Obj types.
typedef enum {
	OBJ_BOUND_METHOD,
	OBJ_CLASS,
	OBJ_CLOSURE,
	OBJ_FUNCTION,
	OBJ_INSTANCE,
	OBJ_NATIVE,
	OBJ_STRING,
	OBJ_UPVALUE,
} ObjType;

struct Obj {
	ObjType type;
	struct Obj* next;
	bool marked;
};

struct ObjString {
	struct Obj obj;
	//
	hash_t hash;
	size_t length;
	char* chars;
};

typedef struct {
	struct Obj obj;
	//
	ObjString* name;
	int arity;
	int upvalues;
	Chunk bytecode;
} ObjFunction;

typedef bool (*NativeFn)(int arc, Value argv[]);

typedef struct {
	struct Obj obj;
	//
	NativeFn function;
} ObjNative;

typedef struct ObjUpvalue {
	struct Obj obj;
	//
	Value* location;
	Value closed;
	struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
	struct Obj obj;
	//
	ObjFunction* function;
	int upvalue_count;
	ObjUpvalue** upvalues;
} ObjClosure;

typedef struct {
	struct Obj obj;
	//
	ObjString* name;
	Table methods;
} ObjClass;

typedef struct {
	struct Obj obj;
	//
	ObjClass* class;
	Table fields;
} ObjInstance;

typedef struct {
	struct Obj obj;
	//
	Value receiver;
	ObjClosure* method;
} ObjBoundMethod;


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

inline bool value_is_closure(Value value)
{
	return value_obj_is_type(value, OBJ_CLOSURE);
}

inline bool value_is_native(Value value)
{
	return value_obj_is_type(value, OBJ_NATIVE);
}

inline bool value_is_class(Value value)
{
	return value_obj_is_type(value, OBJ_CLASS);
}

inline bool value_is_instance(Value value)
{
	return value_obj_is_type(value, OBJ_INSTANCE);
}

inline bool value_is_method(Value value)
{
	return value_obj_is_type(value, OBJ_BOUND_METHOD);
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

inline ObjClosure* value_as_closure(Value value)
{
	return (ObjClosure*)value_as_obj(value);
}

inline ObjClass* value_as_class(Value value)
{
	return (ObjClass*)value_as_obj(value);
}

inline ObjInstance* value_as_instance(Value value)
{
	return (ObjInstance*)value_as_obj(value);
}

inline ObjBoundMethod* value_as_method(Value value)
{
	return (ObjBoundMethod*)value_as_obj(value);
}

void obj_print(Value value);

// Deallocates a single OBJECT from ENV's heap.
void free_obj(struct Environment *env, Obj* object);

// Deallocates all Objs from ENV.
void free_objects(struct Environment *env);

// Allocates a new ObjString while copying given STR.
ObjString* make_obj_string(struct Environment *env, const char* str, size_t str_len);

// Allocates a new ObjString which is the concatenation of PREFIX and SUFFIX.
ObjString* obj_string_concat(struct Environment *env, const ObjString* prefix, const ObjString* sufix);

// Allocates a new ObjFunction in ENV's heap.
ObjFunction* make_obj_function(struct Environment *env);

// Allocates a new ObjNative FUNCTION in ENV's heap.
ObjNative* make_obj_native(struct Environment *env, NativeFn function);

// Allocates a new ObjClojure FUNCTION in ENV's heap.
ObjClosure* make_obj_closure(struct Environment *env, ObjFunction* function);

// Allocates a new ObjUpvalue with SLOT in ENV's heap.
ObjUpvalue* make_obj_upvalue(struct Environment *env, Value* slot);

// Allocates a new ObjClass called NAME in ENV's heap.
ObjClass* make_obj_class(struct Environment *env, ObjString* name);

// Allocates a new ObjInstance of CLASS in ENV's heap.
ObjInstance* make_obj_instance(struct Environment *env, ObjClass* class);

// Allocates a new ObjBoundMethod METHOD bound to RECEIVER.
ObjBoundMethod* make_obj_method(struct Environment *env, Value receiver, ObjClosure* method);

#endif // CLOX_OBJECT_H
