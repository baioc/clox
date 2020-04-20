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
		case OBJ_STRING: printf("%s", value_as_c_str(value)); break;
	}
}

static Obj* allocate_obj(Obj** objects, size_t size, ObjType type)
{
	Obj* obj = malloc(size);
#ifdef DEBUG_DYNAMIC_MEMORY
	printf(";;; Allocating object of type %d at %p\n", type, obj);
#endif

	obj->type = type;

	obj->next = *objects;
	*objects = obj;

	return obj;
}

static ObjString* allocate_string(Obj** objects, char* chars, int length)
{
	ObjString* string = (ObjString*)allocate_obj(objects,
	                                             sizeof(ObjString),
	                                             OBJ_STRING);
	string->length = length;
	string->chars = chars;
	return string;
}

ObjString* obj_string_copy(Obj** objects, const char* chars, int length)
{
	char* heap_chars = malloc(sizeof(char) * length + 1);
	memcpy(heap_chars, chars, length);
	heap_chars[length] = '\0';
#ifdef DEBUG_DYNAMIC_MEMORY
	printf(";;; Allocating string \"%s\" at %p\n", heap_chars, heap_chars);
#endif
	return allocate_string(objects, heap_chars, length);
}

ObjString* obj_string_take(Obj** objects, char* str, int length)
{
	return allocate_string(objects, str, length);
}
