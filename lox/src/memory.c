#include "memory.h"

#include <stdlib.h> // malloc, free, realloc, exit
#include <stdio.h>
#include <assert.h>

#include <sgl/stack.h>

#include "vm.h"
#include "common.h" // DEBUG_LOG_GC
#include "value.h"
#include "object.h"


static void debug_noop(const char* why)
{
#if DEBUG_LOG_GC
	printf("%p allocate 0 for %s\n", NULL, why);
#endif
}

static void debug_free(void* ptr, const char* why)
{
#if DEBUG_LOG_GC
	printf("%p free type %s\n", ptr, why);
#endif
}

static void debug_alloc(void* ptr, size_t size, const char* why)
{
#if DEBUG_LOG_GC
	printf("%p allocate %ld for %s\n", ptr, size, why);
#endif
}

void* reallocate(void* ptr, size_t size, const char* why)
{
	void* mem = NULL;

	if (ptr == NULL && size == 0) {
		debug_noop(why);
		return ptr;
	} else if (ptr == NULL && size != 0) {
		mem = malloc(size);
	} else if (ptr != NULL && size == 0) {
		debug_free(ptr, why);
		free(ptr);
		return ptr;
	} else if (ptr != NULL && size != 0) {
		mem = realloc(ptr, size);
	}

	debug_alloc(mem, size, why);

	if (mem == NULL) {
		fprintf(stderr, "Out of memory for '%s' !\n", why);
		exit(74);
	}

	return mem;
}

static void mark_object(VM* vm, Obj* object)
{
	if (object == NULL || object->marked) return;
#if DEBUG_LOG_GC
	printf("%p mark ", object);
	value_print(obj_value(object));
	printf("\n");
#endif
	object->marked = true;
	stack_push(&vm->data.grays, &object);
	// @TODO: objects which don't hold references don't need to be marked gray
}

static void mark_value(VM* vm, Value value)
{
	if (!value_is_obj(value))
		return;
	else
		mark_object(vm, value_as_obj(value));
}

static void mark_each(const ObjString* key, Value* value, void* vm_ptr)
{
	VM* vm = (VM*)vm_ptr;
	mark_object(vm, (Obj*)key);
	mark_value(vm, *value);
}

static void mark_roots(VM* vm)
{
	// locals
	for (Value* slot = vm->stack; slot < vm->stack_pointer; slot++)
		mark_value(vm, *slot);

	// upvalues
	for (ObjUpvalue* upvalue = vm->data.open_upvalues; upvalue != NULL; upvalue = upvalue->next)
		mark_object(vm, (Obj*)upvalue);

	// globals
	table_for_each(&vm->data.globals, mark_each, vm);

	// call frames
	for (int i = 0; i < vm->frame_count; ++i)
		mark_object(vm, (Obj*)vm->frames[i].subroutine);

	// static data
	// @FIXME: mark_compiler_roots();
	// @FIXME: vm->data.constants not GC'ed ?
	// for (int i = 0; i < value_array_size(&vm->data.constants); ++i)
	// 	mark_value(vm, value_array_get(&vm->data.constants, i));
}

static void blacken_object(VM* vm, Obj* object)
{
#if DEBUG_LOG_GC
	printf("%p blacken ", (void*)object);
	value_print(obj_value(object));
	printf("\n");
#endif
	switch (object->type) {
		case OBJ_CLOSURE: {
			ObjClosure* closure = (ObjClosure*)object;
			mark_object(vm, (Obj*)closure->function);
			for (int i = 0; i < closure->upvalue_count; ++i)
				mark_object(vm, (Obj*)closure->upvalues[i]);
			break;
		}
		case OBJ_FUNCTION:
			mark_object(vm, (Obj*)((ObjFunction*)object)->name);
			break;
		case OBJ_UPVALUE:
			mark_value(vm, ((ObjUpvalue*)object)->closed);
			break;
		case OBJ_NATIVE: case OBJ_STRING:
			break;
		default: assert(false);
	}
}

static void trace_references(VM* vm)
{
	while (!stack_empty(&vm->data.grays)) {
		Obj* object;
		stack_pop(&vm->data.grays, &object);
		blacken_object(vm, object);
	}
}

static void sweep(VM* vm)
{
	Obj* previous = NULL;
	Obj* object = vm->data.objects;
	while (object != NULL) {
		if (object->marked) {
			object->marked = false;
			previous = object;
			object = object->next;
		} else {
			Obj* white = object;
			object = object->next;
			if (previous != NULL) {
				previous->next = object;
			} else {
				vm->data.objects = object;
			}
			free_obj(white);
		}
	}
}

static void remove_each_white(const ObjString* key, Value* value, void* table_ptr)
{
	Table* table = (Table*)table_ptr;
	if (!key->obj.marked) table_delete(table, key);
	/* @NOTE: modifying the table while iterating through it is dangerous, this
	only works because we know the implementation of table_delete/map_delete */
}

void collect_garbage(VM* vm)
{
#if DEBUG_LOG_GC
	printf("-- gc begin\n");
#endif
	mark_roots(vm);
	trace_references(vm);
	table_for_each(&vm->data.strings, remove_each_white, &vm->data.strings);
	sweep(vm);
#if DEBUG_LOG_GC
	printf("-- gc end\n");
#endif
}
