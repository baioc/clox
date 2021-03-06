#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

// Lazy Lox tokenizer.
typedef struct {
	int line;
	const char* start;
	const char* current;
} Scanner;

// Enumeration of valid Lox tokens, plus some extra signaling tokens.
typedef enum {
	// Single-character tokens.
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT,
	TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON,
	TOKEN_SLASH, TOKEN_STAR,

	// One-or-two-character tokens.
	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,

	// Literals.
	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

	// Keywords.
	TOKEN_AND,
	TOKEN_CLASS,
	TOKEN_ELSE,
	TOKEN_FALSE, TOKEN_FOR, TOKEN_FUN,
	TOKEN_IF,
	TOKEN_NIL,
	TOKEN_OR,
	TOKEN_PRINT,
	TOKEN_RETURN,
	TOKEN_SUPER,
	TOKEN_THIS, TOKEN_TRUE,
	TOKEN_VAR,
	TOKEN_WHILE,

	// Signal tokens.
	TOKEN_ERROR,
	TOKEN_EOF,
} TokenType;

// A simple token structure.
typedef struct Token {
	TokenType type;
	int length;
	const char* start;
	int line;
} Token;

// Initializes SCANNER to read from null-terminated string SOURCE.
void scanner_start(Scanner* scanner, const char* source);

// Reads the next Token from SCANNER.
Token scan_token(Scanner* scanner);

#endif // CLOX_SCANNER_H
