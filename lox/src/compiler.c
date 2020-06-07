#include "compiler.h"

#include <stdio.h> // fprintf
#include <stdlib.h> // strtod
#include <assert.h>
#include <string.h> // memcmp

#include <sgl/core.h> // swap

#include "scanner.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "table.h"
#include "common.h" // uint8_t, UINT8_MAX, UINT16_MAX, DEBUG_PRINT_CODE
#ifdef DEBUG_PRINT_CODE
#	include "debug.h" // disassemble_chunk
#endif


typedef struct {
	Token name;
	int depth;
} Local;

typedef enum {
	TYPE_FUNCTION,
	TYPE_SCRIPT,
} FunctionType;

typedef struct {
	ObjFunction* subroutine;
	FunctionType type;
	Local locals[UINT8_MAX + 1];
	int local_count;
	int scope_depth;
} Compiler;

typedef struct {
	Compiler compiler; // syntax-directed translation
	Scanner scanner;
	Token current;
	Token previous;
	bool error;
	bool panic;
	Obj** objects;
	Table* strings;
} Parser;

typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT, // =
	PREC_OR, // or
	PREC_AND, // and
	PREC_EQUALITY, // == !=
	PREC_COMPARISON, // < > <= >=
	PREC_TERM, // + -
	PREC_FACTOR, // * /
	PREC_UNARY, // ! -
	PREC_CALL, // . ()
} Precedence;

typedef void (*ParseFn)(Parser*, bool);
typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;


// forward decls.
static void grouping(Parser* parser, bool can_assign);
static void number(Parser* parser, bool can_assign);
static void unary(Parser* parser, bool can_assign);
static void binary(Parser* parser, bool can_assign);
static void literal(Parser* parser, bool can_assign);
static void string(Parser* parser, bool can_assign);
static void variable(Parser* parser, bool can_assign);
static void and(Parser* parser, bool can_assign);
static void or(Parser* parser, bool can_assign);
static void call(Parser* parser, bool can_assign);

static void declaration(Parser* parser);
static void statement(Parser* parser);
static void expression(Parser* parser);

static ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]    = { grouping, call,   PREC_CALL       },
	[TOKEN_RIGHT_PAREN]   = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_LEFT_BRACE]    = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_RIGHT_BRACE]   = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_COMMA]         = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_DOT]           = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_MINUS]         = { unary,    binary, PREC_TERM       },
	[TOKEN_PLUS]          = { NULL,     binary, PREC_TERM       },
	[TOKEN_SEMICOLON]     = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_SLASH]         = { NULL,     binary, PREC_FACTOR     },
	[TOKEN_STAR]          = { NULL,     binary, PREC_FACTOR     },
	[TOKEN_BANG]          = { unary,    NULL,   PREC_NONE       },
	[TOKEN_BANG_EQUAL]    = { NULL,     binary, PREC_EQUALITY   },
	[TOKEN_EQUAL]         = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_EQUAL_EQUAL]   = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_GREATER]       = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_GREATER_EQUAL] = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_LESS]          = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_LESS_EQUAL]    = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_IDENTIFIER]    = { variable, NULL,   PREC_NONE       },
	[TOKEN_STRING]        = { string,   NULL,   PREC_NONE       },
	[TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE       },
	[TOKEN_AND]           = { NULL,     and,    PREC_AND        },
	[TOKEN_CLASS]         = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_FALSE]         = { literal,  NULL,   PREC_NONE       },
	[TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_IF]            = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_NIL]           = { literal,  NULL,   PREC_NONE       },
	[TOKEN_OR]            = { NULL,     or,     PREC_OR         },
	[TOKEN_PRINT]         = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_RETURN]        = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_SUPER]         = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_THIS]          = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_TRUE]          = { literal,  NULL,   PREC_NONE       },
	[TOKEN_VAR]           = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_WHILE]         = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_ERROR]         = { NULL,     NULL,   PREC_NONE       },
	[TOKEN_EOF]           = { NULL,     NULL,   PREC_NONE       },
};

static ParseRule* get_rule(TokenType type)
{
	return &rules[type];
}

static Chunk* current_chunk(const Parser* p)
{
	return &p->compiler.subroutine->bytecode;
}


static void emit_byte(Parser* parser, uint8_t byte)
{
	chunk_write(current_chunk(parser), byte, parser->previous.line);
}

static void emit_bytes(Parser* parser, uint8_t byte1, uint8_t byte2)
{
	emit_byte(parser, byte1);
	emit_byte(parser, byte2);
}

static void error_at(Parser* parser, const Token* token, const char* message)
{
	if (parser->panic) return;
	else parser->panic = true;

	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF)
		fprintf(stderr, " at end");
	else if (token->type != TOKEN_ERROR)
		fprintf(stderr, " at '%.*s'", token->length, token->start);

	fprintf(stderr, ": %s\n", message);
	parser->error = true;
}

static void error(Parser* parser, const char* message)
{
	error_at(parser, &parser->previous, message);
}

static void error_at_current(Parser* parser, const char* message)
{
	error_at(parser, &parser->current, message);
}

static void advance(Parser* parser)
{
	parser->previous = parser->current;
	for (;;) {
		parser->current = scan_token(&parser->scanner);
		if (parser->current.type != TOKEN_ERROR) break;
		error_at_current(parser, parser->current.start);
	}
}

static bool check(const Parser* parser, TokenType type)
{
	return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type)
{
	if (!check(parser, type)) return false;
	advance(parser);
	return true;
}

static void consume(Parser* parser, TokenType type, const char* message)
{
	if (!match(parser, type))
		error_at_current(parser, message);
}

// Implements the core of Vaughan Pratt's recursive parsing algorithm.
static void parse_with_precedence(Parser* parser, Precedence precedence)
{
	advance(parser); // advances to next token, so previous must be analyzed
	ParseFn prefix_rule = get_rule(parser->previous.type)->prefix;
	if (prefix_rule == NULL) {
		error(parser, "Expect expression.");
		return;
	}
	const bool can_assign = precedence <= PREC_ASSIGNMENT;
	prefix_rule(parser, can_assign);

	// use precedence to check if prefix expression is an operand
	while (get_rule(parser->current.type)->precedence >= precedence) {
		advance(parser);
		ParseFn infix_rule = get_rule(parser->previous.type)->infix;
		infix_rule(parser, can_assign);
	}

	// a token_equal is ignored in the recursion when assigning to an rvalue
	if (can_assign && match(parser, TOKEN_EQUAL))
		error(parser, "Invalid assignment target.");
}

static void expression(Parser* parser)
{
	parse_with_precedence(parser, PREC_ASSIGNMENT);
}

static uint8_t make_constant(Parser* parser, Value value)
{
	unsigned constant_idx = chunk_add_constant(current_chunk(parser), value);
	if (constant_idx > UINT8_MAX) {
		error(parser, "Too many constants in one chunk.");
		return 0;
	}
	return constant_idx;
}

static uint8_t make_string_constant(Parser* parser, const char* str, size_t len)
{
	// check if string isn't registered, and if so return its pool id
	ObjString* var = table_find_string(parser->strings, str, len, table_hash(str, len));
	if (var != NULL) {
		Value val;
		const bool found = table_get(parser->strings, var, &val);
		assert(found);
		return value_as_number(val);
	}

	// make string and associate it with its created constant pool id
	var = make_obj_string(parser->objects, parser->strings, str, len);
	const uint8_t id = make_constant(parser, obj_value((Obj*)var));
	table_put(parser->strings, var, number_value(id));
	return id;
}

static void add_local(Parser* p, Token name)
{
	if (p->compiler.local_count >= UINT8_MAX + 1) {
		error(p, "Too many local variables in function.");
		return;
	}
	Local* local = &p->compiler.locals[p->compiler.local_count++];
	local->name = name;
	local->depth = -1;
}

static bool token_equal(const Token* a, const Token* b)
{
	return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static void declare_variable(Parser* p)
{
	// skip when in global scope: globals are implicitly declared on definition
	if (p->compiler.scope_depth == 0) return;

	// check if there's already a variable with this name in the current scope
	const Token* name = &p->previous;
	for (int i = p->compiler.local_count - 1; i >= 0; --i) {
		const Local* local = &p->compiler.locals[i];
		if (local->depth >= 0 && local->depth < p->compiler.scope_depth)
			break;
		else if (token_equal(name, &local->name))
			error(p, "Variable with this name already declared in this scope.");
	}

	add_local(p, *name);
}

static uint8_t parse_variable(Parser* p, const char* message)
{
	consume(p, TOKEN_IDENTIFIER, message);
	declare_variable(p);
	if (p->compiler.scope_depth > 0)
		return 0; // dummy return
	else
		return make_string_constant(p, p->previous.start, p->previous.length);
}

static void mark_initialized(Parser* p)
{
	if (p->compiler.scope_depth > 0)
		p->compiler.locals[p->compiler.local_count - 1].depth = p->compiler.scope_depth;
}

static void define_variable(Parser* parser, uint8_t var)
{
	mark_initialized(parser);
	if (parser->compiler.scope_depth == 0)
		emit_bytes(parser, OP_DEFINE_GLOBAL, var);
}

static void variable_declaration(Parser* parser)
{
	const uint8_t var = parse_variable(parser, "Expect variable name.");
	if (match(parser, TOKEN_EQUAL))
		expression(parser);
	else
		emit_byte(parser, OP_NIL);

	consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
	define_variable(parser, var);
}

static void expression_statement(Parser* parser)
{
	expression(parser);
	consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
	emit_byte(parser, OP_POP);
}

static void print_statement(Parser* parser)
{
	expression(parser);
	consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
	emit_byte(parser, OP_PRINT);
}

static void return_statement(Parser* parser)
{
	if (parser->compiler.type == TYPE_SCRIPT)
		error(parser, "Cannot return from top-level code.");

	if (match(parser, TOKEN_SEMICOLON)) {
		emit_byte(parser, OP_NIL);
		emit_byte(parser, OP_RETURN);
	} else {
		expression(parser);
		consume(parser, TOKEN_SEMICOLON, "Expect ';' after return value.");
		emit_byte(parser, OP_RETURN);
	}
}

static int emit_jump(Parser* parser, uint8_t instruction)
{
	emit_byte(parser, instruction);
	// @NOTE: jump instructions use 16 bit addresses
	emit_byte(parser, 0xFF);
	emit_byte(parser, 0xFF);
	return chunk_size(current_chunk(parser)) - 2;
}

static void patch_jump(Parser* parser, int address)
{
	const int stride = chunk_size(current_chunk(parser)) - (address + 2);
	if (stride > UINT16_MAX)
		error(parser, "Too much code to jump over.");

	// @NOTE: the Lox VM is big-endian
	chunk_set_byte(current_chunk(parser), address, (stride >> 8) & 0xFF);
	chunk_set_byte(current_chunk(parser), address + 1, stride & 0xFF);
}

static void if_statement(Parser* parser)
{
	// check condition
	consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression(parser);
	consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	// jump over consequence if false
	const int then_jump = emit_jump(parser, OP_JUMP_IF_FALSE);

	// consequence
	emit_byte(parser, OP_POP);
	statement(parser);
	const int else_jump = emit_jump(parser, OP_JUMP);
	patch_jump(parser, then_jump);

	// alternative
	emit_byte(parser, OP_POP);
	if (match(parser, TOKEN_ELSE)) statement(parser);
	patch_jump(parser, else_jump);
}

static void emit_loop(Parser* parser, uint8_t target)
{
	emit_byte(parser, OP_LOOP); // like OP_JUMP, but jumps backwards

	// calculate jump length, needs to acknowledge OP_JUMP's 16-bit operand
	const int stride = chunk_size(current_chunk(parser)) - target + 2;
	if (stride > UINT16_MAX)
		error(parser, "Loop body too large.");

	emit_byte(parser, (stride >> 8) & 0xFF);
	emit_byte(parser, stride & 0xFF);
}

static void while_statement(Parser* parser)
{
	// check condition expression
	const int loop_jump = chunk_size(current_chunk(parser));
	consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression(parser);
	consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	// if false, exit loop
	const int break_jump = emit_jump(parser, OP_JUMP_IF_FALSE);

	// otherwise, pop it and execute loop body
	emit_byte(parser, OP_POP);
	statement(parser);
	emit_loop(parser, loop_jump);
	patch_jump(parser, break_jump);

	// on exit, pop evaluated condition
	emit_byte(parser, OP_POP);
}

static void block(Parser* parser)
{
	while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
		declaration(parser);
	}
	consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void scope_begin(Parser* parser)
{
	parser->compiler.scope_depth++;
}

static void scope_end(Parser* p)
{
	// reduce scope depth and then pop all locals in the previous scope
	p->compiler.scope_depth--;
	while (   p->compiler.local_count > 0
	       && p->compiler.locals[p->compiler.local_count - 1].depth > p->compiler.scope_depth
	      ) {
		// @TODO: optimize scope exit with an instruction to pop all locals
		emit_byte(p, OP_POP);
		p->compiler.local_count--;
	}
}

static void compile_begin(Compiler* compiler, FunctionType type, Obj** objects)
{
	// initialize compilation context
	compiler->subroutine = make_obj_function(objects);
	compiler->type = type;
	compiler->local_count = 0;
	compiler->scope_depth = 0;

	// reserve stack slot 0 (with a voldemort name so it can't be referred to)
	Local* local = &compiler->locals[compiler->local_count++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;
}

static ObjFunction* compile_end(Parser* parser)
{
	// @FIXME: even when there's a return, [OP_NIL OP_RETURN] are emitted
	emit_byte(parser, OP_NIL);
	emit_byte(parser, OP_RETURN);

#ifdef DEBUG_PRINT_CODE
	if (!parser->error) {
		const ObjFunction* proc = parser->compiler.subroutine;
		disassemble_chunk(current_chunk(parser), proc->name == NULL ? "<script>" : proc->name->chars);
	}
#endif

	return parser->compiler.subroutine;
}

static void function(Parser* p, FunctionType type)
{
	// each function has its specific compiler information (and name)
	Compiler compiler;
	compile_begin(&compiler, type, p->objects);
	compiler.subroutine->name = make_obj_string(p->objects, p->strings,
	                                            p->previous.start, p->previous.length);
	swap(&p->compiler, &compiler, sizeof(Compiler));

	// formal argument list
	scope_begin(p);
	consume(p, TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	if (!check(p, TOKEN_RIGHT_PAREN)) {
		do {
			p->compiler.subroutine->arity++;
			if (p->compiler.subroutine->arity > 255) {
				error_at_current(p, "Cannot have more than 255 parameters.");
			}
			const uint8_t param = parse_variable(p, "Expect parameter name.");
			define_variable(p, param);
		} while (match(p, TOKEN_COMMA));
	}
	consume(p, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

	// procedure body
	consume(p, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block(p);

	// create the function object and restore enclosing compilation context
	ObjFunction* function = compile_end(p);
	swap(&p->compiler, &compiler, sizeof(Compiler));
	emit_bytes(p, OP_CONSTANT, make_constant(p, obj_value((Obj*)function)));
}

static void for_statement(Parser* parser)
{
	scope_begin(parser);

	// initialization step
	consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	if (match(parser, TOKEN_SEMICOLON))
		/* no initializer */;
	else if (match(parser, TOKEN_VAR))
		variable_declaration(parser); // variable declared in the for's scope
	else
		expression_statement(parser);

	// condition checking
	int loop_jump = chunk_size(current_chunk(parser));
	int break_jump = -1;
	if (!match(parser, TOKEN_SEMICOLON)) {
		expression(parser);
		consume(parser, TOKEN_SEMICOLON, "Expect ';' after loop condition.");
		// if false, exit the loop
		break_jump = emit_jump(parser, OP_JUMP_IF_FALSE);
		// otherwise, start loop body after popping the evaluated condition
		emit_byte(parser, OP_POP);
	}

	// increment step
	if (!match(parser, TOKEN_RIGHT_PAREN)) {
		// increment is compiled before the actual body, so first must jump
		const int body_jump = emit_jump(parser, OP_JUMP);
		// this is the actual increment code
		const int increment_jump = chunk_size(current_chunk(parser));
		expression(parser);
		emit_byte(parser, OP_POP);
		consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
		emit_loop(parser, loop_jump); // after increment, loop back
		// the body code generate below will jump to this increment step
		loop_jump = increment_jump;
		patch_jump(parser, body_jump);
	}

	// execute body and loop back
	statement(parser);
	emit_loop(parser, loop_jump);

	// the loop exit is only generated when there's a condition
	// @NOTE: this makes for (;;) faster than while (true)
	if (break_jump >= 0) {
		patch_jump(parser, break_jump);
		emit_byte(parser, OP_POP);
	}

	scope_end(parser);
}

static void statement(Parser* parser)
{
	if (match(parser, TOKEN_PRINT)) {
		print_statement(parser);
	} else if (match(parser, TOKEN_RETURN)) {
		return_statement(parser);
	} else if (match(parser, TOKEN_LEFT_BRACE)) {
		scope_begin(parser);
		block(parser);
		scope_end(parser);
	} else if (match(parser, TOKEN_IF)) {
		if_statement(parser);
	} else if (match(parser, TOKEN_WHILE)) {
		while_statement(parser);
	} else if (match(parser, TOKEN_FOR)) {
		for_statement(parser);
	} else {
		expression_statement(parser);
	}
}

/* @FIXME: function references during compilation: can't call a function from another
fun bar(lhs, rhs) { return lhs + rhs; } { fun foo(a, b) { print(bar(a,b) == 3); } var x = 1; var y = bar(x, x); foo(x, y); }
*/
static void function_declaration(Parser* parser)
{
	const uint8_t var = parse_variable(parser, "Expect function name.");
	mark_initialized(parser); // to allow recursion
	function(parser, TYPE_FUNCTION);
	define_variable(parser, var);
}

static void and(Parser* parser, bool can_assign)
{
	// if the left operand is false, leave it on the stack
	const int end_jump = emit_jump(parser, OP_JUMP_IF_FALSE);

	// otherwise, pop it and evaluate the rest of the expression
	emit_byte(parser, OP_POP);
	parse_with_precedence(parser, PREC_AND);
	patch_jump(parser, end_jump);
}

static void or(Parser* parser, bool can_assign)
{
	// if the left operand is false, skip the next jump ...
	const int else_jump = emit_jump(parser, OP_JUMP_IF_FALSE);

	// otherwise (is true), leave it on the stack and skip other sub-expressions
	const int end_jump = emit_jump(parser, OP_JUMP);
	patch_jump(parser, else_jump);

	// ... then pop it and evaluate the rest of the expression
	emit_byte(parser, OP_POP);
	parse_with_precedence(parser, PREC_OR);
	patch_jump(parser, end_jump);
}

static void synchronize(Parser* parser)
{
	parser->panic = false;
	while (parser->current.type != TOKEN_EOF) {
		if (parser->previous.type == TOKEN_SEMICOLON) return;
		switch (parser->current.type) {
			case TOKEN_CLASS:
			case TOKEN_FUN:
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_PRINT:
			case TOKEN_RETURN:
			return;
			default: /* Keep looking for sync point. */ ;
		}
		advance(parser);
	}
}

static void declaration(Parser* parser)
{
	if (match(parser, TOKEN_VAR))
		variable_declaration(parser);
	else if (match(parser, TOKEN_FUN))
		function_declaration(parser);
	else
		statement(parser);

	if (parser->panic)
		synchronize(parser);
}

static void number(Parser* parser, bool can_assign)
{
	const double value = strtod(parser->previous.start, NULL);
	emit_bytes(parser, OP_CONSTANT, make_constant(parser, number_value(value)));
}

static void grouping(Parser* parser, bool can_assign)
{
	expression(parser);
	consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(Parser* parser, bool can_assign)
{
	const TokenType operator_type = parser->previous.type;

	// compile the operand
	parse_with_precedence(parser, PREC_UNARY);

	// emit instruction defined by operator
	switch (operator_type) {
		case TOKEN_BANG: emit_byte(parser, OP_NOT); break;
		case TOKEN_MINUS: emit_byte(parser, OP_NEGATE); break;
	}
}

static void binary(Parser* parser, bool can_assign)
{
	// remember operator
	const TokenType operator_type = parser->previous.type;

	// compile the right operand
	parse_with_precedence(parser, get_rule(operator_type)->precedence + 1);

	// emit operator instruction
	switch (operator_type) {
		case TOKEN_BANG_EQUAL: emit_bytes(parser, OP_EQUAL, OP_NOT); break;
		case TOKEN_EQUAL_EQUAL: emit_byte(parser, OP_EQUAL); break;
		case TOKEN_GREATER: emit_byte(parser, OP_GREATER); break;
		case TOKEN_GREATER_EQUAL: emit_bytes(parser, OP_LESS, OP_NOT); break;
		case TOKEN_LESS: emit_byte(parser, OP_LESS); break;
		case TOKEN_LESS_EQUAL: emit_bytes(parser, OP_GREATER, OP_NOT); break;
		case TOKEN_PLUS: emit_byte(parser, OP_ADD); break;
		case TOKEN_MINUS: emit_byte(parser, OP_SUBTRACT); break;
		case TOKEN_STAR: emit_byte(parser, OP_MULTIPLY); break;
		case TOKEN_SLASH: emit_byte(parser, OP_DIVIDE); break;
	}
}

static uint8_t argument_list(Parser* parser)
{
	uint8_t argc = 0;
	if (!check(parser, TOKEN_RIGHT_PAREN)) {
		do {
			expression(parser);
			++argc;
			if (argc >= 255) error(parser, "Cannot have more than 255 arguments.");
		} while (match(parser, TOKEN_COMMA));
	}
	consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
	return argc;
}

static void call(Parser* parser, bool can_assign)
{
	const uint8_t argc = argument_list(parser);
	emit_bytes(parser, OP_CALL, argc);
}

static void literal(Parser* parser, bool can_assign)
{
	switch (parser->previous.type) {
		case TOKEN_FALSE: emit_byte(parser, OP_FALSE); break;
		case TOKEN_NIL: emit_byte(parser, OP_NIL); break;
		case TOKEN_TRUE: emit_byte(parser, OP_TRUE); break;
	}
}

static void string(Parser* parser, bool can_assign)
{
	// remember to account for starting and closing quotes '"'
	const char* string = parser->previous.start + 1;
	const size_t length = parser->previous.length - 2;
	const uint8_t id = make_string_constant(parser, string, length);
	emit_bytes(parser, OP_CONSTANT, id);
}

static int resolve_local(Parser* p, const Token* name)
{
	// name lookup going from the top to the bottom of the locals stack
	for (int i = p->compiler.local_count - 1; i >= 0; --i) {
		const Local* local = &p->compiler.locals[i];
		if (token_equal(name, &local->name)) {
			if (local->depth < 0)
				error(p, "Cannot read local variable in its own initializer.");
			return i;
		}
	}
	return -1;
}

static void named_variable(Parser* parser, const Token* name, bool can_assign)
{
	uint8_t get_op, set_op;
	int id = resolve_local(parser, name);
	if (id >= 0) {
		get_op = OP_GET_LOCAL;
		set_op = OP_SET_LOCAL;
	} else {
		id = make_string_constant(parser, name->start, name->length);
		get_op = OP_GET_GLOBAL;
		set_op = OP_SET_GLOBAL;
	}

	if (can_assign && match(parser, TOKEN_EQUAL)) {
		expression(parser);
		emit_bytes(parser, set_op, (uint8_t)id);
	} else {
		emit_bytes(parser, get_op, (uint8_t)id);
	}
}

static void variable(Parser* parser, bool can_assign)
{
	named_variable(parser, &parser->previous, can_assign);
}

static void register_string_constant(const ObjString* key, Value* val, void* p)
{
	const uint8_t id = make_constant((Parser*)p, obj_value((Obj*)key));
	*val = number_value(id);
}

ObjFunction* compile(const char* source, Obj** objects, Table* strings)
{
	// begin compilation
	Parser parser = {
		.error = false,
		.panic = false,
		.objects = objects,
		.strings = strings,
	};
	compile_begin(&parser.compiler, TYPE_SCRIPT, objects);

	// register any previously interned strings into the chunk's constant pool
	table_for_each(strings, register_string_constant, &parser);

	// setup scanner to have both .current and .next
	scanner_start(&parser.scanner, source);
	advance(&parser);

	// a "compilation unit" that translates into a chunk is a sequence of decls.
	while (!match(&parser, TOKEN_EOF))
		declaration(&parser);

	ObjFunction* proc = compile_end(&parser);
	return parser.error ? NULL : proc;
}
