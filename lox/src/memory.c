#include "memory.h"

#include <stdlib.h> // malloc, free, realloc, exit
#include <stdio.h>
#include <assert.h>

#include <sgl/stack.h>
#include <sgl/core.h> // byte_t

#include "vm.h"
#include "common.h" // DEBUG_LOG_GC
#include "value.h"
#include "object.h"
#include "compiler.h" // Compiler


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

static void* sized_malloc(Environment* env, size_t size)
{
	byte_t* base = malloc(sizeof(size_t) + size);
	if (base == NULL) return NULL;
	byte_t* block = base + sizeof(size_t);
	*(size_t*)base = size;
	env->allocated += size;
	return block;
}

static void sized_free(Environment* env, void* ptr)
{
	if (ptr == NULL) return;
	byte_t* block = ptr;
	byte_t* base = block - sizeof(size_t);
	const size_t size = *(size_t*)base;
	free(base);
	env->allocated -= size;
}

static void* sized_realloc(Environment* env, void* ptr, size_t new_size)
{
	if (ptr == NULL) return sized_malloc(env, new_size);

	byte_t* block = ptr;
	byte_t* base = block - sizeof(size_t);
	const size_t old_size = *(size_t*)base;
	if (old_size >= new_size) return ptr;

	byte_t* new_base = realloc(base, sizeof(size_t) + new_size);
	if (new_base == NULL) return NULL;

	base = new_base;
	block = base + sizeof(size_t);
	*(size_t*)base = new_size;
	env->allocated += new_size - old_size;
	return block;
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

	void* mem = NULL;

	if (ptr == NULL && size == 0) {
		debug_noop(why);
		return ptr;
	} else if (ptr == NULL && size != 0) {
		mem = sized_malloc(env, size);
	} else if (ptr != NULL && size == 0) {
		debug_free(ptr, why);
		sized_free(env, ptr);
		return ptr;
	} else if (ptr != NULL && size != 0) {
		mem = sized_realloc(env, ptr, size);
	}

	debug_alloc(mem, size, why);
	if (mem == NULL) {
		fprintf(stderr, "Out of memory for '%s'!\n", why);
		exit(74);
	}

#if DEBUG_STRESS_GC
	collect_garbage();
#else
	if (env->allocated > env->next_gc)
		collect_garbage();
#endif

	return mem;
}

static void mark_object(Environment* env, Obj* object)
{
	if (object == NULL || object->marked) return;

	object->marked = true;
#if DEBUG_LOG_GC
	printf("%p mark ", object);
	value_print(obj_value(object));
	printf("\n");
#endif

	// objects which don't hold references don't need to be marked gray
	if (object->type == OBJ_NATIVE || object->type == OBJ_STRING)
		return;
	else
		stack_push(&env->grays, &object);
}

static void mark_value(Environment* env, Value value)
{
	if (!value_is_obj(value))
		return;
	else
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
			object = object->next;
			if (previous != NULL) {
				previous->next = object;
			} else {
				env->objects = object;
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
