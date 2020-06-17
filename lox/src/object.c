#include "object.h"

#include <stdio.h>
#include <string.h> // memcpy
#include <stddef.h> // size_t
#include <assert.h>

#include "table.h"
#include "chunk.h"
#include "memory.h" // reallocate


extern inline ObjType obj_type(Value value);

extern inline bool value_obj_is_type(Value value, ObjType type);
extern inline bool value_is_string(Value value);
extern inline bool value_is_function(Value value);
extern inline bool value_is_closure(Value value);
extern inline bool value_is_native(Value value);
extern inline bool value_is_class(Value value);
extern inline bool value_is_instance(Value value);

extern inline ObjString* value_as_string(Value value);
extern inline char* value_as_c_str(Value value);
extern inline ObjFunction* value_as_function(Value value);
extern inline ObjClosure* value_as_closure(Value value);
extern inline NativeFn value_as_native(Value value);
extern inline ObjClass* value_as_class(Value value);
extern inline ObjInstance* value_as_instance(Value value);

static void print_function(ObjFunction* function)
{
	if (function->name == NULL)
		printf("<script>");
	else
		printf("<fn %s>", function->name->chars);
}

void obj_print(Value value)
{
	switch (obj_type(value)) {
		case OBJ_STRING:
			printf("\"%s\"", value_as_c_str(value));
			break;
		case OBJ_FUNCTION:
			print_function(value_as_function(value));
			break;
		case OBJ_CLOSURE:
			print_function(value_as_closure(value)->function);
			break;
		case OBJ_UPVALUE:
			printf("upvalue");
			break;
		case OBJ_NATIVE:
			printf("<native fn>");
			break;
		case OBJ_CLASS:
			printf("%s", value_as_class(value)->name->chars);
			break;
		case OBJ_INSTANCE:
			printf("%s instance", value_as_instance(value)->class->name->chars);
			break;
		default:
			assert(false);
	}
}

void free_obj(Obj* object)
{
	#define FREE_OBJ(object, type) reallocate((object), 0, #type)

	switch (object->type) {
		case OBJ_STRING: {
			ObjString* string = (ObjString*)object;
			reallocate(string->chars, 0, "string");
			FREE_OBJ(object, ObjString);
			break;
		}
		case OBJ_FUNCTION:
			chunk_destroy(&((ObjFunction*)object)->bytecode);
			FREE_OBJ(object, ObjFunction);
			break;
		case OBJ_CLOSURE: {
			ObjClosure* closure = (ObjClosure*)object;
			reallocate(closure->upvalues, 0, "upvalues[]");
			FREE_OBJ(object, ObjClosure);
			break;
		}
		case OBJ_UPVALUE: FREE_OBJ(object, ObjUpvalue); break;
		case OBJ_NATIVE: FREE_OBJ(object, ObjNative); break;
		case OBJ_CLASS: FREE_OBJ(object, ObjClass); break;
		case OBJ_INSTANCE: {
			ObjInstance* instance = (ObjInstance*)object;
			table_destroy(&instance->fields);
			FREE_OBJ(object, ObjInstance);
			break;
		}
		default: assert(false);
	}

	#undef FREE_OBJ
}

void free_objects(Obj** objects)
{
	#define HEAD(list) (*(list))

	while (HEAD(objects) != NULL) {
		Obj* next = HEAD(objects)->next;
		free_obj(HEAD(objects));
		HEAD(objects) = next;
	}

	#undef HEAD
}

static Obj* allocate_obj(Obj** objects, size_t size, ObjType type, const char* why)
{
	Obj* obj = reallocate(NULL, size, why);
	obj->type = type;
	obj->next = *objects;
	obj->marked = false;
	*objects = obj;
	return obj;
}

#define ALLOCATE_OBJ(objects, type, enum_type) \
	(type*)allocate_obj((objects), sizeof(type), (enum_type), #type);

static ObjString* make_obj_string_copy(Obj** objects, Table* strings,
                                       const char* str, size_t n, hash_t hash)
{
	char* chars = reallocate(NULL, n + 1, "string");
	memcpy(chars, str, n);
	chars[n] = '\0';

	ObjString* string = ALLOCATE_OBJ(objects, ObjString, OBJ_STRING);
	string->hash = hash;
	string->length = n;
	string->chars = chars;

	string->obj.marked = true;
	table_put(strings, string, nil_value());
	string->obj.marked = false;

	return string;
}

ObjString* make_obj_string(Obj** objs, Table* strings, const char* str, size_t n)
{
	const hash_t hash = table_hash(str, n);
	ObjString* interned = table_find_string(strings, str, n, hash);
	return interned != NULL ? interned
	                        : make_obj_string_copy(objs, strings, str, n, hash);
}

ObjString* obj_string_concat(Obj** objs, Table* strings,
                             const ObjString* prefix, const ObjString* suffix)
{
	// prepare temporary resultant string for hash
	const size_t n = prefix->length + suffix->length;
	char str[n]; // VLA
	memcpy(str, prefix->chars, prefix->length);
	memcpy(str + prefix->length, suffix->chars, suffix->length);

	const hash_t hash = table_hash(str, n);
	ObjString* interned = table_find_string(strings, str, n, hash);
	return interned != NULL ? interned
	                        : make_obj_string_copy(objs, strings, str, n, hash);
}

ObjFunction* make_obj_function(Obj** objects)
{
	ObjFunction* proc = ALLOCATE_OBJ(objects, ObjFunction, OBJ_FUNCTION);
	proc->arity = 0;
	proc->upvalues = 0;
	proc->name = NULL;
	chunk_init(&proc->bytecode); // initialized with 0 size, so proc is GC safe
	return proc;
}

ObjNative* make_obj_native(Obj** objects, NativeFn function)
{
	ObjNative* native = ALLOCATE_OBJ(objects, ObjNative, OBJ_NATIVE);
	native->function = function;
	return native;
}

ObjClosure* make_obj_closure(Obj** objects, ObjFunction* function)
{
	const size_t size = sizeof(ObjUpvalue*) * function->upvalues;
	ObjUpvalue** upvalues = reallocate(NULL, size, "upvalues[]");
	for (int i = 0; i < function->upvalues; ++i)
		upvalues[i] = NULL;

	ObjClosure* closure = ALLOCATE_OBJ(objects, ObjClosure, OBJ_CLOSURE);
	closure->function = function;
	closure->upvalues = upvalues;
	closure->upvalue_count = function->upvalues;

	return closure;
}

ObjUpvalue* make_obj_upvalue(Obj** objects, Value* slot)
{
	ObjUpvalue* upvalue = ALLOCATE_OBJ(objects, ObjUpvalue, OBJ_UPVALUE);
	upvalue->location = slot;
	upvalue->closed = nil_value();
	upvalue->next = NULL;
	return upvalue;
}

ObjClass* make_obj_class(Obj** objects, ObjString* name)
{
	ObjClass* class = ALLOCATE_OBJ(objects, ObjClass, OBJ_CLASS);
	class->name = name;
	return class;
}

ObjInstance* make_obj_instance(Obj** objects, ObjClass* class)
{
	ObjInstance* instance = ALLOCATE_OBJ(objects, ObjInstance, OBJ_INSTANCE);
	instance->class = class;
	table_init(&instance->fields); // initialized with 0 size, so it is GC safe
	return instance;
}

#undef ALLOCATE_OBJ
