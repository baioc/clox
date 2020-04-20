#include "object.h"

#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // memcpy
#include <stddef.h> // size_t

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

static Obj* allocate_obj(Obj** objects, size_t size, ObjType type)
{
	Obj* obj = malloc(size);
#ifdef DEBUG_DYNAMIC_MEMORY
	printf(";;; Allocating object %d (%u bytes) at %p\n", type, size, obj);
#endif

	obj->type = type;

	obj->next = *objects;
	*objects = obj;

	return obj;
}

ObjString* make_obj_string(Obj** objects, int length)
{
	const size_t size = sizeof(ObjString) + length + 1;
	ObjString* string = (ObjString*)allocate_obj(objects, size, OBJ_STRING);
	string->length = length;
	return string;
}

ObjString* make_obj_string_copy(Obj** objects, const char* c_str, int length)
{
	ObjString* string = make_obj_string(objects, length);
	memcpy(string->chars, c_str, length);
	string->chars[length] = '\0';
	return string;
}
