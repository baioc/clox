#include "vm.h"

#include <stdio.h>
#include <stdarg.h> // varargs
#include <string.h> // strlen
#include <time.h> // clock(), CLOCKS_PER_SEC
#include <assert.h>

#include <ugly/core.h> // ARRAY_SIZE
#include <ugly/stack.h>

#include "chunk.h"
#include "value.h"
#include "object.h" // free_objects
#include "compiler.h"
#include "table.h"
#include "common.h" // GC_HEAP_INITIAL
#if DEBUG_TRACE_EXECUTION
#	include "debug.h" // disassemble_instruction
#endif


static Environment* env = NULL;

Environment* lox_getenv(void)
{
	return env;
}

Environment* lox_setenv(Environment* new)
{
	Environment* old = env;
	env = new;
	return old;
}

int constant_add(ValueArray* constants, Value value)
{
	const int index = value_array_size(constants);
	if (value_is_obj(value)) value_as_obj(value)->marked = true;
	value_array_write(constants, value);
	if (value_is_obj(value)) value_as_obj(value)->marked = false;
	return index;
}

inline Value constant_get(const ValueArray* constants, uint8_t index)
{
	return value_array_get(constants, index);
}

static void reset_stack(VM* vm)
{
	vm->stack_pointer = vm->stack;
	vm->frame_count = 0;
}

static void define_native(VM* vm, const char* name, NativeFn function);

static bool native_clock(int argc, Value argv[])
{
	if (argc != 0) return false;
	argv[-1] = number_value((double)clock() / CLOCKS_PER_SEC);
	return true;
}

static bool native_error(int argc, Value argv[])
{
	if (argc == 1)
		argv[-1] = argv[0];
	return false;
}

static bool native_hasField(int argc, Value argv[])
{
	if (argc != 2) return false;
	else if (!value_is_instance(argv[0])) return false;
	else if (!value_is_string(argv[1])) return false;

	ObjInstance* instance = value_as_instance(argv[0]);
	const ObjString* field = value_as_string(argv[1]);
	Value dummy;
	argv[-1] = bool_value(table_get(&instance->fields, field, &dummy));
	return true;
}

static bool native_getField(int argc, Value argv[])
{
	if (argc != 2) return false;
	else if (!value_is_instance(argv[0])) return false;
	else if (!value_is_string(argv[1])) return false;

	ObjInstance* instance = value_as_instance(argv[0]);
	const ObjString* field = value_as_string(argv[1]);
	table_get(&instance->fields, field, &argv[-1]);
	return true;
}

static bool native_setField(int argc, Value argv[])
{
	if (argc != 3) return false;
	else if (!value_is_instance(argv[0])) return false;
	else if (!value_is_string(argv[1])) return false;

	ObjInstance* instance = value_as_instance(argv[0]);
	const ObjString* field = value_as_string(argv[1]);
	table_put(&instance->fields, field, argv[2]);
	argv[-1] = argv[2];
	return true;
}

static bool native_deleteField(int argc, Value argv[])
{
	if (argc != 2) return false;
	else if (!value_is_instance(argv[0])) return false;
	else if (!value_is_string(argv[1])) return false;

	ObjInstance* instance = value_as_instance(argv[0]);
	const ObjString* field = value_as_string(argv[1]);
	table_delete(&instance->fields, field);
	return true;
}

void vm_init(VM* vm)
{
	lox_setenv(&vm->data);
	vm->data.vm = vm;
	vm->data.compiler = NULL;

	reset_stack(vm);
	vm->data.open_upvalues = NULL;
	vm->data.allocated = 0;
	vm->data.next_gc = GC_HEAP_INITIAL;

	stack_init(&vm->data.grays, 0, sizeof(Obj*), NULL);
	vm->init_string = NULL;
	vm->data.objects = NULL;

	value_array_init(&vm->data.constants);
	table_init(&vm->data.strings);
	table_init(&vm->data.globals);
	vm->init_string = make_obj_string(&vm->data.objects, &vm->data.strings, "init", 4);

	define_native(vm, "clock", native_clock);
	define_native(vm, "error", native_error);
	define_native(vm, "hasField", native_hasField);
	define_native(vm, "getField", native_getField);
	define_native(vm, "setField", native_setField);
	define_native(vm, "deleteField", native_deleteField);
}

void vm_destroy(VM* vm)
{
	table_destroy(&vm->data.globals);
	table_destroy(&vm->data.strings);
	value_array_destroy(&vm->data.constants);
	free_objects(&vm->data.objects);
	vm->init_string = NULL;
	stack_destroy(&vm->data.grays);
	vm->data.vm = NULL;
}

static void push(VM* vm, Value value)
{
	*vm->stack_pointer = value;
	vm->stack_pointer++;
}

static Value pop(VM* vm)
{
	vm->stack_pointer--;
	return *vm->stack_pointer;
}

static Value peek(const VM* vm, int distance)
{
	return vm->stack_pointer[-1 - distance];
}

static bool value_is_falsey(Value value)
{
	return value_is_nil(value)
	    || (value_is_bool(value) && value_as_bool(value) == false);
}

static void concatenate_strings(VM* vm)
{
	const ObjString* b = value_as_string(peek(vm, 0));
	const ObjString* a = value_as_string(peek(vm, 1));
	ObjString* c = obj_string_concat(&vm->data.objects, &vm->data.strings, a, b);
	pop(vm);
	pop(vm);
	push(vm, obj_value((Obj*)c));
}

static void runtime_error(VM* vm, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	for (int i = vm->frame_count - 1; i >= 0; --i) {
		const CallFrame* frame = &vm->frames[i];
		const ObjFunction* function = frame->subroutine->function;

		// loop backwards on operands until we find a bytecode with a valid line
		intptr_t offset = frame->program_counter;
		int line = -1;
		do {
			offset -= 1;
			line = chunk_get_line(&function->bytecode, offset);
		} while (offset > 0 && line < 0);

		fprintf(stderr, "[line %d] in ", line);
		if (function->name == NULL)
			fprintf(stderr, "script\n");
		else
			fprintf(stderr, "%s()\n", function->name->chars);
	}

	reset_stack(vm);
}

static void define_native(VM* vm, const char* name, NativeFn function)
{
	push(vm, obj_value((Obj*)make_obj_string(&vm->data.objects, &vm->data.strings,
	                                         name, strlen(name))));
	push(vm, obj_value((Obj*)make_obj_native(&vm->data.objects, function)));
	table_put(&vm->data.globals, value_as_string(peek(vm, 1)), peek(vm, 0));
	pop(vm);
	pop(vm);
}

static bool call(VM* vm, ObjClosure* closure, int argc)
{
	if (argc != closure->function->arity) {
		runtime_error(vm, "Expected %d arguments but got %d.", closure->function->arity, argc);
		return false;
	} else if (vm->frame_count >= ARRAY_SIZE(vm->frames)) {
		runtime_error(vm, "Stack overflow.");
		return false;
	} else {
		CallFrame* frame = &vm->frames[vm->frame_count++];
		frame->subroutine = closure;
		frame->program_counter = 0;
		frame->frame_pointer = vm->stack_pointer - (argc + 1);
		return true;
	}
}

static bool call_value(VM* vm, Value callee, int argc)
{
	if (!value_is_obj(callee))
		goto FAIL;

	switch (obj_type(callee)) {
		case OBJ_CLOSURE:
			return call(vm, value_as_closure(callee), argc);
		case OBJ_NATIVE: {
			NativeFn native = value_as_native(callee);
			vm->stack_pointer[- argc - 1] = nil_value();
			if (native(argc, vm->stack_pointer - argc)) {
				vm->stack_pointer -= argc;
				return true;
			} else {
				const Value err = vm->stack_pointer[- argc - 1];
				if (value_is_string(err))
					runtime_error(vm, "Error: %s", value_as_c_str(err));
				else
					runtime_error(vm, "Error!");
			}
			return false;
		}
		case OBJ_CLASS: { // calling a class evokes its constructor
			ObjClass* class = value_as_class(callee);
			ObjInstance* instance = make_obj_instance(&vm->data.objects, class);
			vm->stack_pointer[-(argc + 1)] = obj_value((Obj*)instance);
			Value initializer;
			if (table_get(&class->methods, vm->init_string, &initializer)) {
				return call(vm, value_as_closure(initializer), argc);
			} else if (argc != 0) {
				runtime_error(vm, "Expected 0 arguments but got %d.", argc);
				return false;
			} else {
				/* default initializer */
				return true;
			}
		}
		case OBJ_BOUND_METHOD: {
			ObjBoundMethod* bound = value_as_method(callee);
			vm->stack_pointer[-(argc + 1)] = bound->receiver;
			return call(vm, bound->method, argc);
		}
		default:
			goto FAIL;
	}

FAIL:
	runtime_error(vm, "Can only call functions and classes.");
	return false;
}

static bool invoke_from_class(VM* vm, ObjClass* class, ObjString* name, int argc)
{
	Value method;
	if (!table_get(&class->methods, name, &method)) {
		runtime_error(vm, "Undefined property '%s'.", name->chars);
		return false;
	}
	return call(vm, value_as_closure(method), argc);
}

static bool invoke(VM* vm, ObjString* name, int argc)
{
	const Value receiver = peek(vm, argc);
	if (!value_is_instance(receiver)) {
		runtime_error(vm, "Only instances have methods.");
		return false;
	}
	ObjInstance* instance = value_as_instance(receiver);

	// check if we're invoking a field instead of a method
	Value value;
	if (table_get(&instance->fields, name, &value)) {
		vm->stack_pointer[-(argc + 1)] = value;
		return call_value(vm, value, argc);
	}

	return invoke_from_class(vm, instance->class, name, argc);
}

static ObjUpvalue* capture_upvalue(VM* vm, Value* local)
{
	// before creating an upvalue variable, look up an existing one for sharing
	ObjUpvalue* prev = NULL;
	ObjUpvalue* curr = vm->data.open_upvalues;

	// the open upvalues list is sorted from the stack top
	while (curr != NULL && curr->location > local) {
		prev = curr;
		curr = curr->next;
	}
	if (curr != NULL && curr->location == local)
		return curr;

	// otherwise, create a upvalue and insert it into the sorted list
	ObjUpvalue* upvalue = make_obj_upvalue(&vm->data.objects, local);
	upvalue->next = curr;
	if (prev != NULL) {
		prev->next = upvalue;
	} else {
		vm->data.open_upvalues = upvalue;
	}
	return upvalue;
}

static void close_upvalues(VM* vm, Value* last)
{
	while (vm->data.open_upvalues != NULL && vm->data.open_upvalues->location >= last) {
		ObjUpvalue* upvalue = vm->data.open_upvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm->data.open_upvalues = upvalue->next;
	}
}

static void define_method(VM* vm, ObjString* name)
{
	const Value method = peek(vm, 0);
	ObjClass* class = value_as_class(peek(vm, 1));
	table_put(&class->methods, name, method);
	pop(vm);
}

static bool bind_method(VM* vm, ObjClass* class, ObjString* name)
{
	Value meth;
	if (!table_get(&class->methods, name, &meth)) {
		return false;
	}
	ObjBoundMethod* bound = make_obj_method(&vm->data.objects,
	                                        peek(vm, 0), value_as_closure(meth));
	pop(vm);
	push(vm, obj_value((Obj*)bound));
	return true;
}

static void debug_trace_run(const VM* vm, const CallFrame* frame)
{
#if DEBUG_TRACE_EXECUTION
	printf(" /------> ");
	for (const Value* slot = vm->stack; slot < vm->stack_pointer; slot++) {
		printf("[ ");
		value_print(*slot);
		printf(" ]");
	}
	printf("\n");
	disassemble_instruction(&frame->subroutine->function->bytecode,
	                        &vm->data.constants,
	                        frame->program_counter);
#endif
}

static void inherit_each_method(const ObjString* name, Value* method, void* class_ptr)
{
	ObjClass* class = (ObjClass*)class_ptr;
	table_put(&class->methods, name, *method);
}

static InterpretResult run(VM* vm)
{
	CallFrame* frame = &vm->frames[vm->frame_count - 1];

	#define READ_BYTE() \
		chunk_get_byte(&frame->subroutine->function->bytecode, frame->program_counter++)
	#define READ_SHORT() \
		(frame->program_counter += 2, \
		 (chunk_get_byte(&frame->subroutine->function->bytecode, frame->program_counter - 2) << 8) \
		| chunk_get_byte(&frame->subroutine->function->bytecode, frame->program_counter - 1))
	#define READ_CONSTANT() constant_get(&vm->data.constants, READ_BYTE())
	#define READ_STRING() value_as_string(READ_CONSTANT())
	#define BINARY_OP(type_value, op) do { \
		if (!value_is_number(peek(vm, 0)) || !value_is_number(peek(vm, 1))) { \
			runtime_error(vm, "Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		const double b = value_as_number(pop(vm)); \
		const double a = value_as_number(pop(vm)); \
		push(vm, type_value(a op b)); \
	} while (0)

	// virtual machine fetch-decode-execute loop
	for (;;) {
		debug_trace_run(vm, frame);
		const uint8_t instruction = READ_BYTE();

		// @TODO: benchmark manual jump table optimized with computed gotos
		switch (instruction) {
			case OP_CONSTANT:
				push(vm, READ_CONSTANT());
				break;

			case OP_NIL:
				push(vm, nil_value());
				break;

			case OP_TRUE:
				push(vm, bool_value(true));
				break;

			case OP_FALSE:
				push(vm, bool_value(false));
				break;

			case OP_POP:
				pop(vm);
				break;

			case OP_GET_LOCAL: {
				const uint8_t slot = READ_BYTE();
				push(vm, frame->frame_pointer[slot]);
				break;
			}

			case OP_SET_LOCAL: {
				const uint8_t slot = READ_BYTE();
				// assignment is an expression -> no need to pop assigned value
				frame->frame_pointer[slot] = peek(vm, 0);
				break;
			}

			case OP_GET_GLOBAL: {
				/* @NOTE: we could optimize access to globals via statically
				computed array indexes instead of hash table name lookups */
				const ObjString* name = READ_STRING();
				Value value;
				if (!table_get(&vm->data.globals, name, &value)) {
					runtime_error(vm, "Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				push(vm, value);
				break;
			}

			case OP_DEFINE_GLOBAL:
				table_put(&vm->data.globals, READ_STRING(), peek(vm, 0));
				pop(vm);
				break;

			case OP_SET_GLOBAL: {
				ObjString* name = READ_STRING();
				if (!table_put(&vm->data.globals, name, peek(vm, 0))) {
					table_delete(&vm->data.globals, name);
					runtime_error(vm, "Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}

			case OP_GET_UPVALUE: {
				const uint8_t slot = READ_BYTE();
				push(vm, *frame->subroutine->upvalues[slot]->location);
				break;
			}

			case OP_SET_UPVALUE: {
				const uint8_t slot = READ_BYTE();
				*frame->subroutine->upvalues[slot]->location = peek(vm, 0);
				break;
			}

			case OP_GET_PROPERTY: {
				if (!value_is_instance(peek(vm, 0))) {
					runtime_error(vm, "Only instances have properties.");
					return INTERPRET_RUNTIME_ERROR;
				}

				const ObjInstance* instance = value_as_instance(peek(vm, 0));
				ObjString* name = READ_STRING();

				Value value;
				if (table_get(&instance->fields, name, &value)) {
					pop(vm);
					push(vm, value);
				} else if (!bind_method(vm, instance->class, name)) {
					runtime_error(vm, "Undefined property '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}

			case OP_SET_PROPERTY: {
				if (!value_is_instance(peek(vm, 1))) {
					runtime_error(vm, "Only instances have fields.");
					return INTERPRET_RUNTIME_ERROR;
				}

				ObjInstance* instance = value_as_instance(peek(vm, 1));
				ObjString* name = READ_STRING();
				table_put(&instance->fields, name, peek(vm, 0));

				Value value = pop(vm);
				pop(vm);
				push(vm, value);
				break;
			}

			case OP_GET_SUPER: {
				ObjString* name = READ_STRING();
				ObjClass* super = value_as_class(pop(vm));
				if (!bind_method(vm, super, name))
					return INTERPRET_RUNTIME_ERROR;
				break;
			}

			case OP_EQUAL: {
				const Value b = pop(vm);
				const Value a = pop(vm);
				push(vm, bool_value(value_equal(a, b)));
				break;
			}

			case OP_GREATER:
				BINARY_OP(bool_value, >);
				break;

			case OP_LESS:
				BINARY_OP(bool_value, <);
				break;

			case OP_ADD:
				if (value_is_string(peek(vm, 0)) && value_is_string(peek(vm, 1))) {
					concatenate_strings(vm);
				} else if (!value_is_number(peek(vm, 0)) || !value_is_number(peek(vm, 1))) {
					runtime_error(vm, "Operands must be two numbers or two strings.");
					return INTERPRET_RUNTIME_ERROR;
				} else {
					BINARY_OP(number_value, +);
				}
				break;

			case OP_SUBTRACT:
				BINARY_OP(number_value, -);
				break;

			case OP_MULTIPLY:
				BINARY_OP(number_value, *);
				break;

			case OP_DIVIDE:
				BINARY_OP(number_value, /);
				break;

			case OP_NOT:
				push(vm, bool_value(value_is_falsey(pop(vm))));
				break;

			case OP_NEGATE:
				if (!value_is_number(peek(vm, 0))) {
					runtime_error(vm, "Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}
				push(vm, number_value(-value_as_number(pop(vm))));
				break;

			case OP_PRINT:
				value_print(pop(vm));
				printf("\n");
				break;

			case OP_JUMP: {
				const uint16_t jump = READ_SHORT();
				frame->program_counter += jump;
				break;
			}

			case OP_JUMP_IF_FALSE: {
				const uint16_t jump = READ_SHORT();
				if (value_is_falsey(peek(vm, 0)))
					frame->program_counter += jump;
				break;
			}

			case OP_LOOP: {
				const uint16_t jump = READ_SHORT();
				frame->program_counter -= jump;
				break;
			}

			case OP_CALL: {
				const int argc = READ_BYTE();
				const Value callee = peek(vm, argc);
				if (!call_value(vm, callee, argc)) {
					return INTERPRET_RUNTIME_ERROR;
				}
				frame = &vm->frames[vm->frame_count - 1];
				break;
			}

			case OP_INVOKE: {
				ObjString* method = READ_STRING();
				const int argc = READ_BYTE();
				if (!invoke(vm, method, argc)) {
					return INTERPRET_RUNTIME_ERROR;
				}
				frame = &vm->frames[vm->frame_count - 1];
				break;
			}

			case OP_SUPER_INVOKE: {
				ObjString* method = READ_STRING();
				const int argc = READ_BYTE();
				ObjClass* super = value_as_class(pop(vm));
				if (!invoke_from_class(vm, super, method, argc)) {
					return INTERPRET_RUNTIME_ERROR;
				}
				frame = &vm->frames[vm->frame_count - 1];
				break;
			}

			case OP_CLOSURE: {
				ObjFunction* function = value_as_function(READ_CONSTANT());
				ObjClosure* closure = make_obj_closure(&vm->data.objects, function);
				push(vm, obj_value((Obj*)closure));
				for (int i = 0; i < closure->upvalue_count; ++i) {
					const uint8_t local = READ_BYTE();
					const uint8_t index = READ_BYTE();
					closure->upvalues[i] = local ? capture_upvalue(vm, frame->frame_pointer + index)
					                             : frame->subroutine->upvalues[index];
				}
				break;
			}

			case OP_CLOSE_UPVALUE:
				close_upvalues(vm, vm->stack_pointer - 1);
				pop(vm);
				break;

			case OP_RETURN: {
				const Value result = pop(vm);

				close_upvalues(vm, frame->frame_pointer);

				vm->frame_count--;
				if (vm->frame_count <= 0) {
					pop(vm);
					return INTERPRET_OK;
				}

				vm->stack_pointer = frame->frame_pointer;
				push(vm, result);

				frame = &vm->frames[vm->frame_count - 1];
				break;
			}

			case OP_CLASS: {
				ObjString* name = READ_STRING();
				ObjClass* class = make_obj_class(&vm->data.objects, name);
				push(vm, obj_value((Obj*)class));
				break;
			}

			case OP_INHERIT: {
				const Value super = peek(vm, 1);
				if (!value_is_class(super)) {
					runtime_error(vm, "Superclass must be a class.");
					return INTERPRET_RUNTIME_ERROR;
				}
				const ObjClass* superclass = value_as_class(super);
				ObjClass* class = value_as_class(peek(vm, 0));
				table_for_each(&superclass->methods, inherit_each_method, class);
				pop(vm); // subclass
				// no need to pop super as it is on a separate scope
				break;
			}

			case OP_METHOD:
				define_method(vm, READ_STRING());
				break;

			default:
				runtime_error(vm, "Invalid opcode %d\n", instruction);
				return INTERPRET_COMPILE_ERROR;
		}
	}

	#undef BINARY_OP
	#undef READ_STRING
	#undef READ_CONSTANT
	#undef READ_SHORT
	#undef READ_BYTE
}

InterpretResult vm_interpret(VM* vm, const char* source)
{
	ObjFunction* main = compile(source, &vm->data);
	if (main == NULL) return INTERPRET_COMPILE_ERROR;

	// setup initial call frame
	push(vm, obj_value((Obj*)main));
	ObjClosure* program = make_obj_closure(&vm->data.objects, main);
	pop(vm);
	push(vm, obj_value((Obj*)program));
	call(vm, program, 0);

	return run(vm);
}
