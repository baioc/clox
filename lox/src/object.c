#include "object.h"

#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <stddef.h> // size_t
#include <assert.h>

#include "common.h" // DEBUG_DYNAMIC_MEMORY
#include "table.h"
#include "chunk.h"


#if DEBUG_DYNAMIC_MEMORY
#	define DEBUG_FREE_OBJ(obj, type, size) \
		printf(";;; Freeing %s (%u bytes) at %p\n", #type, size, obj)

#	define DEBUG_MALLOC_OBJ(obj, type, size) \
		printf(";;; Allocating [%d] object (%u bytes) at %p\n", type, size, obj)

#	define DEBUG_FREE_EXTRA(obj, extra_ptr, type_str, size) do { \
			printf(";;; Freeing %s ", type_str); \
			obj_print(obj_value(obj)); \
			printf(" (%u bytes) at %p\n", size, extra_ptr); \
		} while (0)

#	define DEBUG_MALLOC_EXTRA(extra_ptr, type_str, size) \
		printf(";;; Allocating %s (%u bytes) at %p\n", type_str, size, extra_ptr)

#else
#	define DEBUG_FREE_OBJ(obj, type, size) ((void)0)
#	define DEBUG_MALLOC_OBJ(obj, type, size) ((void)0)
#	define DEBUG_FREE_EXTRA(obj, extra, type_str, size) ((void)0)
#	define DEBUG_MALLOC_EXTRA(extra_ptr, type_str, size) ((void)0)
#endif


extern inline ObjType obj_type(Value value);

extern inline bool value_obj_is_type(Value value, ObjType type);

extern inline bool value_is_string(Value value);

extern inline bool value_is_function(Value value);

extern inline bool value_is_native(Value value);

extern inline ObjString* value_as_string(Value value);

extern inline char* value_as_c_str(Value value);

extern inline ObjFunction* value_as_function(Value value);

extern inline NativeFn value_as_native(Value value);

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
		case OBJ_STRING: printf("\"%s\"", value_as_c_str(value)); break;
		case OBJ_FUNCTION: print_function(value_as_function(value)); break;
		case OBJ_NATIVE: printf("<native fn>"); break;
		default: assert(false);
	}
}

static void free_obj(Obj* object)
{
	switch (object->type) {
		case OBJ_STRING: {
			ObjString* string = (ObjString*)object;
			DEBUG_FREE_EXTRA(object, string->chars, "string", string->length + 1);
			free(string->chars);
			DEBUG_FREE_OBJ(object, ObjString, sizeof(ObjString));
			free(string);
			break;
		}
		case OBJ_FUNCTION: {
			ObjFunction* function = (ObjFunction*)object;
			chunk_destroy(&function->bytecode);
			DEBUG_FREE_OBJ(object, ObjFunction, sizeof(ObjFunction));
			free(function);
			break;
		}
		case OBJ_NATIVE:
			DEBUG_FREE_OBJ(object, ObjNative, sizeof(ObjNative));
			free(object);
			break;
		default: assert(false);
	}
}

void free_objects(Obj** objects)
{
	#define HEAD(list) (*list)

	while (HEAD(objects) != NULL) {
		Obj* next = HEAD(objects)->next;
		free_obj(HEAD(objects));
		HEAD(objects) = next;
	}

	#undef HEAD
}

static Obj* allocate_obj(Obj** objects, size_t size, ObjType type)
{
	Obj* obj = malloc(size);
	DEBUG_MALLOC_OBJ(obj, type, size);
	if (obj == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(74);
	}

	obj->type = type;
	obj->next = *objects;
	*objects = obj;

	return obj;
}

// Allocates an ObjString and automatically interns it.
static ObjString* allocate_string(Obj** objects, Table* interned,
                                  char* str, size_t str_len, hash_t hash)
{
	const size_t size = sizeof(ObjString);
	ObjString* string = (ObjString*)allocate_obj(objects, size, OBJ_STRING);

	string->hash = hash;
	string->length = str_len;
	string->chars = str;

	table_put(interned, string, nil_value());
	return string;
}

static ObjString* make_obj_string_copy(Obj** objs, Table* strings,
                                       const char* str, size_t n, hash_t hash)
{
	// allocate characters
	char* chars = malloc(n + 1);
	DEBUG_MALLOC_EXTRA(chars, "string", n + 1);
	if (chars == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(74);
	}

	// copy contents
	memcpy(chars, str, n);
	chars[n] = '\0';

	// allocate Lox object
	return allocate_string(objs, strings, chars, n, hash);
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
	char str[n];
	memcpy(str, prefix->chars, prefix->length);
	memcpy(str + prefix->length, suffix->chars, suffix->length);

	const hash_t hash = table_hash(str, n);
	ObjString* interned = table_find_string(strings, str, n, hash);
	return interned != NULL ? interned
	                        : make_obj_string_copy(objs, strings, str, n, hash);
}

ObjFunction* make_obj_function(Obj** objs)
{
	ObjFunction* proc = (ObjFunction*)allocate_obj(objs, sizeof(ObjFunction), OBJ_FUNCTION);
	proc->arity = 0;
	proc->name = NULL;
	chunk_init(&proc->bytecode);
	return proc;
}

ObjNative* make_obj_native(Obj** objects, NativeFn function)
{
	ObjNative* native = (ObjNative*)allocate_obj(objects, sizeof(ObjNative), OBJ_NATIVE);
	native->function = function;
	return native;
}


#undef DEBUG_MALLOC_EXTRA
#undef DEBUG_FREE_EXTRA
#undef DEBUG_MALLOC_OBJ
#undef DEBUG_FREE_OBJ
