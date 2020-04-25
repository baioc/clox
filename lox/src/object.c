#include "object.h"

#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <stddef.h> // size_t

#include <sgl/hash.h> // fnv_1a

#include "common.h" // DEBUG_DYNAMIC_MEMORY


extern inline ObjType obj_type(Value value);

extern inline bool value_obj_is_type(Value value, ObjType type);

extern inline bool value_is_string(Value value);

extern inline ObjString* value_as_string(Value value);

extern inline char* value_as_c_str(Value value);

void obj_print(Value value)
{
	switch (obj_type(value)) {
		case OBJ_STRING: printf("\"%s\"", value_as_c_str(value)); break;
	}
}

static void free_obj(Obj* object)
{
	switch (object->type) {
		case OBJ_STRING: {
			ObjString* string = (ObjString*)object;

		#ifdef DEBUG_DYNAMIC_MEMORY
			printf(";;; Freeing string \"%s\" (%u bytes) at %p\n",
			       string->chars, string->length + 1, string->chars);
		#endif
			free(string->chars);

		#ifdef DEBUG_DYNAMIC_MEMORY
			printf(";;; Freeing <string> object (%u bytes) at %p\n",
			       sizeof(ObjString), string);
		#endif
			free(string);

			break;
		}
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
#ifdef DEBUG_DYNAMIC_MEMORY
	printf(";;; Allocating <%d> object (%u bytes) at %p\n", type, size, obj);
#endif
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

ObjString* make_obj_string(Obj** objs, Table* strings, const char* str, size_t n)
{
	// compute hash and check if already interned
	const hash_t hash = fnv_1a(str, n);
	ObjString* interned = table_find_string(strings, str, n, hash);
	if (interned != NULL) return interned;

	// allocate characters
	char* chars = malloc(n + 1);
#ifdef DEBUG_DYNAMIC_MEMORY
	printf(";;; Allocating string \"%.*s\" (%u bytes) at %p\n",
	       n, str, n + 1, chars);
#endif
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

ObjString* obj_string_concat(Obj** objects, Table* strings,
                             const ObjString* prefix, const ObjString* suffix)
{
	// prepare resultant string
	const size_t n = prefix->length + suffix->length;
	char chars[n];
	memcpy(chars, prefix->chars, prefix->length);
	memcpy(chars + prefix->length, suffix->chars, suffix->length);

	// compute hash and check if already interned
	const hash_t hash = fnv_1a(chars, n);
	ObjString* interned = table_find_string(strings, chars, n, hash);
	if (interned != NULL) return interned;

	// allocate Lox string object
	return make_obj_string(objects, strings, chars, n);
}
