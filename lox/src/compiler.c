#include "compiler.h"

#include <stdio.h> // fprintf
#include <stdlib.h> // strtod

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

typedef void (*ParseFn)(Parser*);
typedef struct {
	ParseFn    prefix;
	ParseFn    infix;
	Precedence precedence;
} ParseRule;


// forward decls.
static void grouping(Parser* parser);
static void number(Parser* parser);
static void unary(Parser* parser);
static void binary(Parser* parser);
static void literal(Parser* parser);
static void string(Parser* parser);

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
	[TOKEN_IDENTIFIER]    = { NULL,     NULL,   PREC_NONE },
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

static void consume(Parser* parser, TokenType type, const char* message)
{
	if (parser->current.type == type)
		advance(parser);
	else
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
	prefix_rule(parser);

	// use precedence to check if prefix expression is an operand
	while (get_rule(parser->current.type)->precedence >= precedence) {
		advance(parser);
		ParseFn infix_rule = get_rule(parser->previous.type)->infix;
		infix_rule(parser);
	}
}

static void expression(Parser* parser)
{
	parse_precedence(parser, PREC_ASSIGNMENT);
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

static void emit_constant(Parser* parser, Value value)
{
	emit_bytes(parser, OP_CONSTANT, make_constant(parser, value));
}

static void number(Parser* parser)
{
	const double value = strtod(parser->previous.start, NULL);
	emit_constant(parser, number_value(value));
}

static void grouping(Parser* parser)
{
	expression(parser);
	consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(Parser* parser)
{
	const TokenType operator_type = parser->previous.type;

	// compile the operand
	parse_precedence(parser, PREC_UNARY);

	// emit instruction defined by operator
	switch (operator_type) {
		case TOKEN_BANG: emit_byte(parser, OP_NOT); break;
		case TOKEN_MINUS: emit_byte(parser, OP_NEGATE); break;
	}
}

static void binary(Parser* parser)
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

static void literal(Parser* parser)
{
	switch (parser->previous.type) {
		case TOKEN_FALSE: emit_byte(parser, OP_FALSE); break;
		case TOKEN_NIL: emit_byte(parser, OP_NIL); break;
		case TOKEN_TRUE: emit_byte(parser, OP_TRUE); break;
	}
}

static void string(Parser* parser)
{
	const ObjString* str = make_obj_string(parser->objects, parser->strings,
	                                       parser->previous.start + 1,
	                                       parser->previous.length - 2);
	emit_constant(parser, obj_value((Obj*)str));
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

bool compile(const char* source, Chunk* chunk, Obj** objects, Table* strings)
{
	Parser parser = {
		.chunk = chunk,
		.error = false,
		.panic = false,
		.objects = objects,
		.strings = strings,
	};
	scanner_start(&parser.scanner, source);

	advance(&parser); // sets up parser->previous for expression
	expression(&parser);
	consume(&parser, TOKEN_EOF, "Expect end of expression.");
	end_compilation(&parser);
	debug_print_compiled(&parser);

	return !parser.error;
}
