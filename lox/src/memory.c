#include "memory.h"

#include "object.h" // ObjType
#include "value.h" // ObjString
#include "common.h" // DEBUG_DYNAMIC_MEMORY
#ifdef DEBUG_DYNAMIC_MEMORY
#	include <stdio.h>
#endif

static void free_obj(Obj* object)
{
	switch (object->type) {
		case OBJ_STRING: {
			ObjString* string = (ObjString*)object;
		#ifdef DEBUG_DYNAMIC_MEMORY
			printf(";;; Freeing object %d (%u bytes) at %p\n",
			       OBJ_STRING, sizeof(ObjString) + string->length + 1, string);
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
