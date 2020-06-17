#include "memory.h"

#include <stdlib.h> // malloc, free, realloc, exit
#include <stdio.h>
#include <assert.h>

#include <sgl/stack.h>
#include <sgl/core.h> // byte_t, CONTAINER_OF

#include "vm.h"
#include "common.h" // DEBUG_LOG_GC
#include "value.h"
#include "object.h"
#include "compiler.h" // Compiler


struct allocation {
	size_t size;
	byte_t block[];
};

static void* sized_malloc(Environment* env, size_t size, const char* why)
{
	if (size == 0)
		return NULL;

	struct allocation* memory = malloc(sizeof(struct allocation) + size);
	if (memory == NULL)
		return NULL;

	memory->size = size;

	env->allocated += size;
#if DEBUG_LOG_GC
	printf("%p allocate %ld for %s\n", &memory->block, size, why);
#endif

	return &memory->block;
}

static void sized_free(Environment* env, void* ptr, const char* why)
{
	if (ptr == NULL)
		return;

	struct allocation* allocation = CONTAINER_OF(ptr, struct allocation, block);
	const size_t size = allocation->size;
	free(allocation);

	env->allocated -= size;
#if DEBUG_LOG_GC
	printf("%p free %ld for %s\n", ptr, size, why);
#endif
}

static void* sized_realloc(Environment* env, void* ptr, size_t size, const char* why)
{
	if (ptr == NULL) {
		return sized_malloc(env, size, why);
	} else if (size == 0) {
		sized_free(env, ptr, why);
		return NULL;
	}

	struct allocation* allocation = CONTAINER_OF(ptr, struct allocation, block);
	const size_t old_size = allocation->size;
	if (size <= old_size)
		return ptr;

	struct allocation* new = realloc(allocation, sizeof(struct allocation) + size);
	if (new == NULL)
		return NULL;

	new->size = size;

	env->allocated += size - old_size;
#if DEBUG_LOG_GC
	printf("%p grow %ld for %s\n", &new->block, size - old_size, why);
#endif

	return &new->block;
}

void* reallocate(void* ptr, size_t size, const char* why)
{
	Environment* env = get_current_lox_environment();
	if (env == NULL) {
	#if DEBUG_LOG_GC
		fprintf(stderr, "GC allocator used for '%s' with no environment!\n", why);
	#endif
		return realloc(ptr, size);
	}

	void* mem = sized_realloc(env, ptr, size, why);
	if (size != 0 && mem == NULL) {
		fprintf(stderr, "Out of memory for '%s'!\n", why);
		exit(74);
	}

#if DEBUG_STRESS_GC
	if (size != 0) // only invoke GC on allocations
		collect_garbage();
#else
	if (env->allocated > env->next_gc)
		collect_garbage();
#endif

	return mem;
}

static void mark_object(Environment* env, Obj* object)
{
	if (object == NULL || object->marked)
		return;

	object->marked = true;
#if DEBUG_LOG_GC
	printf("%p mark ", object);
	value_print(obj_value(object));
	printf("\n");
#endif

	// objects which don't hold references don't need to be traced
	if (object->type == OBJ_NATIVE || object->type == OBJ_STRING)
		return;
	else
		stack_push(&env->grays, &object);
}

static void mark_value(Environment* env, Value value)
{
	if (value_is_obj(value))
		mark_object(env, value_as_obj(value));
}

static void mark_each(const ObjString* key, Value* value, void* env_ptr)
{
	Environment* env = (Environment*)env_ptr;
	mark_object(env, (Obj*)key);
	mark_value(env, *value);
}

static void mark_roots(Environment* env)
{
	// locals
	for (Value* slot = env->vm->stack; slot < env->vm->stack_pointer; slot++)
		mark_value(env, *slot);

	// upvalues
	for (ObjUpvalue* upvalue = env->open_upvalues; upvalue != NULL; upvalue = upvalue->next)
		mark_object(env, (Obj*)upvalue);

	// globals
	table_for_each(&env->globals, mark_each, env);

	// call frames
	for (int i = 0; i < env->vm->frame_count; ++i)
		mark_object(env, (Obj*)env->vm->frames[i].subroutine);

	// static data
	for (int i = 0, n = value_array_size(&env->constants); i < n; ++i)
		mark_value(env, value_array_get(&env->constants, i));

	// compilation data
	for (Compiler* current = env->compiler; current != NULL; current = current->enclosing)
		mark_object(env, (Obj*)current->subroutine);
}

static void blacken_object(Environment* env, Obj* object)
{
#if DEBUG_LOG_GC
	printf("%p blacken ", (void*)object);
	value_print(obj_value(object));
	printf("\n");
#endif
	switch (object->type) {
		case OBJ_CLOSURE: {
			ObjClosure* closure = (ObjClosure*)object;
			mark_object(env, (Obj*)closure->function);
			for (int i = 0; i < closure->upvalue_count; ++i)
				mark_object(env, (Obj*)closure->upvalues[i]);
			break;
		}
		case OBJ_FUNCTION:
			mark_object(env, (Obj*)((ObjFunction*)object)->name);
			break;
		case OBJ_UPVALUE:
			mark_value(env, ((ObjUpvalue*)object)->closed);
			break;
		case OBJ_NATIVE: case OBJ_STRING:
			break;
		case OBJ_CLASS:
			mark_object(env, (Obj*)((ObjClass*)object)->name);
			break;
		case OBJ_INSTANCE: {
			ObjInstance* instance = (ObjInstance*)object;
			mark_object(env, (Obj*)instance->class);
			table_for_each(&instance->fields, mark_each, env);
			break;
		}
		default: assert(false);
	}
}

static void trace_references(Environment* env)
{
	while (!stack_empty(&env->grays)) {
		Obj* object;
		stack_pop(&env->grays, &object);
		blacken_object(env, object);
	}
}

static void sweep(Environment* env)
{
	Obj* previous = NULL;
	Obj* object = env->objects;
	while (object != NULL) {
		if (object->marked) {
			object->marked = false;
			previous = object;
			object = object->next;
		} else {
			Obj* white = object;
			free_obj(white);
			object = object->next;
			if (previous != NULL)
				previous->next = object;
			else
				env->objects = object;
		}
	}
}

static void remove_each_white(const ObjString* key, Value* value, void* table_ptr)
{
	Table* table = (Table*)table_ptr;
	if (!key->obj.marked)
		table_delete(table, key);
	/* @NOTE: modifying the table while iterating through it is dangerous, this
	only works because we know the implementation of table_delete/map_delete */
}

void collect_garbage()
{
	Environment* env = get_current_lox_environment();
	if (env == NULL) return;

#if DEBUG_LOG_GC
	printf("-- gc begin\n");
	const size_t before = env->allocated;
#endif

	mark_roots(env);
	trace_references(env);
	table_for_each(&env->strings, remove_each_white, &env->strings);
	sweep(env);
	env->next_gc = env->allocated * 2; // GC heap grow factor

#if DEBUG_LOG_GC
	printf("-- gc end\n");
	const size_t after = env->allocated;
	printf("   collected %ld bytes (from %ld to %ld) next at %ld\n",
	       before - after, before, after, env->next_gc);
#endif
}
