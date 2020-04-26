#include "compiler.h"

#include <stdio.h> // fprintf
#include <stdlib.h> // strtod
#include <assert.h>

#include "scanner.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "table.h"
#include "common.h" // uint8_t, DEBUG_PRINT_CODE
#ifdef DEBUG_PRINT_CODE
#	include "debug.h" // disassemble_chunk
#endif


typedef struct {
	Scanner scanner;
	Token   current;
	Token   previous;
	bool    error;
	bool    panic;
	Chunk*  chunk;
	Obj**   objects;
	Table*  strings;
} Parser;

typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT,  // =
	PREC_OR,          // or
	PREC_AND,         // and
	PREC_EQUALITY,    // == !=
	PREC_COMPARISON,  // < > <= >=
	PREC_TERM,        // + -
	PREC_FACTOR,      // * /
	PREC_UNARY,       // ! -
	PREC_CALL,        // . ()
} Precedence;

typedef void (*ParseFn)(Parser*, bool);
typedef struct {
	ParseFn    prefix;
	ParseFn    infix;
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

static ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]    = { grouping, NULL,   PREC_NONE },
	[TOKEN_RIGHT_PAREN]   = { NULL,     NULL,   PREC_NONE },
	[TOKEN_LEFT_BRACE]    = { NULL,     NULL,   PREC_NONE },
	[TOKEN_RIGHT_BRACE]   = { NULL,     NULL,   PREC_NONE },
	[TOKEN_COMMA]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_DOT]           = { NULL,     NULL,   PREC_NONE },
	[TOKEN_MINUS]         = { unary,    binary, PREC_TERM },
	[TOKEN_PLUS]          = { NULL,     binary, PREC_TERM },
	[TOKEN_SEMICOLON]     = { NULL,     NULL,   PREC_NONE },
	[TOKEN_SLASH]         = { NULL,     binary, PREC_FACTOR },
	[TOKEN_STAR]          = { NULL,     binary, PREC_FACTOR },
	[TOKEN_BANG]          = { unary,    NULL,   PREC_NONE },
	[TOKEN_BANG_EQUAL]    = { NULL,     binary, PREC_EQUALITY },
	[TOKEN_EQUAL]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_EQUAL_EQUAL]   = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_GREATER]       = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_GREATER_EQUAL] = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_LESS]          = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_LESS_EQUAL]    = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_IDENTIFIER]    = { variable, NULL,   PREC_NONE },
	[TOKEN_STRING]        = { string,   NULL,   PREC_NONE },
	[TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE },
	[TOKEN_AND]           = { NULL,     NULL,   PREC_NONE },
	[TOKEN_CLASS]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE },
	[TOKEN_FALSE]         = { literal,  NULL,   PREC_NONE },
	[TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE },
	[TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE },
	[TOKEN_IF]            = { NULL,     NULL,   PREC_NONE },
	[TOKEN_NIL]           = { literal,  NULL,   PREC_NONE },
	[TOKEN_OR]            = { NULL,     NULL,   PREC_NONE },
	[TOKEN_PRINT]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_RETURN]        = { NULL,     NULL,   PREC_NONE },
	[TOKEN_SUPER]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_THIS]          = { NULL,     NULL,   PREC_NONE },
	[TOKEN_TRUE]          = { literal,  NULL,   PREC_NONE },
	[TOKEN_VAR]           = { NULL,     NULL,   PREC_NONE },
	[TOKEN_WHILE]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_ERROR]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_EOF]           = { NULL,     NULL,   PREC_NONE },
};

static ParseRule* get_rule(TokenType type)
{
	return &rules[type];
}


static void emit_byte(Parser* parser, uint8_t byte)
{
	chunk_write(parser->chunk, byte, parser->previous.line);
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
static void parse_precedence(Parser* parser, Precedence precedence)
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
	parse_precedence(parser, PREC_ASSIGNMENT);
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

static void statement(Parser* parser)
{
	if (match(parser, TOKEN_PRINT))
		print_statement(parser);
	else
		expression_statement(parser);
}

static uint8_t make_constant(Parser* parser, Value value)
{
	unsigned constant_idx = chunk_add_constant(parser->chunk, value);
	if (constant_idx > UINT8_MAX) {
		error(parser, "Too many constants in one chunk.");
		return 0;
	}
	return constant_idx;
}

static uint8_t make_string_constant(Parser* parser, const char* str, size_t len)
{
	// check if string isn't registered, and if so return its pool id
	ObjString* var = table_find_string(parser->strings, str, len,
	                                   table_hash(str, len));
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

static uint8_t parse_variable(Parser* p, const char* message)
{
	consume(p, TOKEN_IDENTIFIER, message);
	return make_string_constant(p, p->previous.start, p->previous.length);
}

static void variable_declaration(Parser* parser)
{
	const uint8_t global = parse_variable(parser, "Expect variable name.");
	if (match(parser, TOKEN_EQUAL))
		expression(parser);
	else
		emit_byte(parser, OP_NIL);

	consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
	emit_bytes(parser, OP_DEFINE_GLOBAL, global);
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
	parse_precedence(parser, PREC_UNARY);

	// emit instruction defined by operator
	switch (operator_type) {
		case TOKEN_BANG:  emit_byte(parser, OP_NOT); break;
		case TOKEN_MINUS: emit_byte(parser, OP_NEGATE); break;
	}
}

static void binary(Parser* parser, bool can_assign)
{
	// remember operator
	const TokenType operator_type = parser->previous.type;

	// compile the right operand
	parse_precedence(parser, get_rule(operator_type)->precedence + 1);

	// emit operator instruction
	switch (operator_type) {
		case TOKEN_BANG_EQUAL:    emit_bytes(parser, OP_EQUAL, OP_NOT); break;
		case TOKEN_EQUAL_EQUAL:   emit_byte(parser, OP_EQUAL); break;
		case TOKEN_GREATER:       emit_byte(parser, OP_GREATER); break;
		case TOKEN_GREATER_EQUAL: emit_bytes(parser, OP_LESS, OP_NOT); break;
		case TOKEN_LESS:          emit_byte(parser, OP_LESS); break;
		case TOKEN_LESS_EQUAL:    emit_bytes(parser, OP_GREATER, OP_NOT); break;
		case TOKEN_PLUS:          emit_byte(parser, OP_ADD); break;
		case TOKEN_MINUS:         emit_byte(parser, OP_SUBTRACT); break;
		case TOKEN_STAR:          emit_byte(parser, OP_MULTIPLY); break;
		case TOKEN_SLASH:         emit_byte(parser, OP_DIVIDE); break;
	}
}

static void literal(Parser* parser, bool can_assign)
{
	switch (parser->previous.type) {
		case TOKEN_FALSE: emit_byte(parser, OP_FALSE); break;
		case TOKEN_NIL:   emit_byte(parser, OP_NIL); break;
		case TOKEN_TRUE:  emit_byte(parser, OP_TRUE); break;
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

static void named_variable(Parser* parser, const Token* name, bool can_assign)
{
	const uint8_t id = make_string_constant(parser, name->start, name->length);
	if (can_assign && match(parser, TOKEN_EQUAL)) {
		expression(parser);
		emit_bytes(parser, OP_SET_GLOBAL, id);
	} else {
		emit_bytes(parser, OP_GET_GLOBAL, id);
	}
}

static void variable(Parser* parser, bool can_assign)
{
	named_variable(parser, &parser->previous, can_assign);
}

static void emit_return(Parser* parser)
{
	emit_byte(parser, OP_RETURN);
}

static void end_compilation(Parser* parser)
{
	emit_return(parser);
}

static void debug_print_compiled(const Parser* parser)
{
#ifdef DEBUG_PRINT_CODE
	if (!parser->error)
		disassemble_chunk(parser->chunk, "code");
#endif
}

static void register_string_constant(const ObjString* key, Value val, void* p)
{
	Parser* parser = (Parser*)p;
	const uint8_t id = make_constant(parser, obj_value((Obj*)key));
	table_put(parser->strings, key, number_value(id));
}

bool compile(const char* source, Chunk* chunk, Obj** objects, Table* strings)
{
	Parser parser = {
		.error = false,
		.panic = false,
		.chunk = chunk,
		.objects = objects,
		.strings = strings,
	};

	// register any previous given strings into the chunk's constant pool
	table_for_each(strings, register_string_constant, &parser);

	scanner_start(&parser.scanner, source);
	advance(&parser); // sets up parser->previous for expression

	// a "compile unit" that translates into a chunk is a sequence of decls.
	while (!match(&parser, TOKEN_EOF))
		declaration(&parser);

	end_compilation(&parser);
	debug_print_compiled(&parser);

	return !parser.error;
}
