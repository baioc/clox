#include "scanner.h"

#include <string.h> // strlen, memcmp

#include "common.h" // bool


void scanner_start(Scanner* scanner, const char* source)
{
	scanner->start = source;
	scanner->current = source;
	scanner->line = 1;
}

// Peeks at the current character without consuming it.
static char peek(const Scanner* scanner)
{
	return scanner->current[0];
}

static bool at_end(const Scanner* scanner)
{
	return peek(scanner) == '\0';
}

static Token make_token(const Scanner* scanner, TokenType type)
{
	return (Token){
		.type = type,
		.start = scanner->start,
		.length = scanner->current - scanner->start,
		.line = scanner->line,
	};
}

static Token error_token(const Scanner* scanner, const char* message)
{
	return (Token){
		.type = TOKEN_ERROR,
		.start = message,
		.length = strlen(message),
		.line = scanner->line,
	};
}

// Consume the current character and returns it.
static char advance(Scanner* scanner)
{
	scanner->current++;
	return scanner->current[-1];
}

// Like peek(), but for one character past the current one.
static char peek_next(const Scanner* scanner) {
	if (at_end(scanner)) return '\0';
	else return scanner->current[1];
}

// Skips all whitespace and comments until the start of the next token.
static void skip_whitespace(Scanner* scanner)
{
	for (;;) {
		const char c = peek(scanner);
		switch (c) {
			case '\n':
				scanner->line++;
				// fallthrough
			case ' ': case '\t': case '\r':
				advance(scanner);
				break;
			case '/':
				if (peek_next(scanner) == '/') {
					// A comment goes until the end of the line.
					while (peek(scanner) != '\n' && !at_end(scanner))
						advance(scanner);
				} else {
					return;
				}
				break;
			default:
				return;
		}
	}
}

// Conditionally consume current character when it matches an expected value.
static bool match(Scanner* scanner, char expected)
{
	if (at_end(scanner)) {
		return false;
	} else if (*(scanner->current) != expected) {
		return false;
	} else {
		scanner->current++;
		return true;
	}
}

static Token scan_string(Scanner* scanner)
{
	while (peek(scanner) != '"' && !at_end(scanner)) {
		if (peek(scanner) == '\n') scanner->line++;
		advance(scanner);
	}

	if (at_end(scanner))
		return error_token(scanner, "Unterminated string.");

	advance(scanner); // consume closing quote
	return make_token(scanner, TOKEN_STRING);
}

static bool is_digit(char c)
{
	return '0' <= c && c <= '9';
}

static Token scan_number(Scanner* scanner)
{
	while (is_digit(peek(scanner)))
		advance(scanner);

	// look for a fractional part
	if (peek(scanner) == '.' && is_digit(peek_next(scanner))) {
		advance(scanner); // consume the "."
		while (is_digit(peek(scanner)))
			advance(scanner);
	}

	return make_token(scanner, TOKEN_NUMBER);
}

static bool is_alpha(char c) {
	return ('a' <= c && c <= 'z')
	    || ('A' <= c && c <= 'Z')
	    || c == '_';
}

static TokenType check_keyword(const Scanner* scanner,
                               int start,
                               const char* expected,
                               int length,
                               TokenType type)
{
	const int current_len = scanner->current - scanner->start;
	const int keyword_len = start + length;
	const char* got = scanner->start + start;
	return current_len == keyword_len && memcmp(got, expected, length) == 0
	       ? type
	       : TOKEN_IDENTIFIER;
}

static TokenType identifier_type(const Scanner* s)
{
	// implements the lexer Trie
	switch (s->start[0]) {
		case 'a': return check_keyword(s, 1, "nd", 2, TOKEN_AND);
		case 'c': return check_keyword(s, 1, "lass", 4, TOKEN_CLASS);
		case 'e': return check_keyword(s, 1, "lse", 3, TOKEN_ELSE);
		case 'f':
			if (s->current - s->start > 1) {
				switch (s->start[1]) {
					case 'a': return check_keyword(s, 2, "lse", 3, TOKEN_FALSE);
					case 'o': return check_keyword(s, 2, "r", 1, TOKEN_FOR);
					case 'u': return check_keyword(s, 2, "n", 1, TOKEN_FUN);
				}
			}
			break;
		case 'i': return check_keyword(s, 1, "f", 1, TOKEN_IF);
		case 'n': return check_keyword(s, 1, "il", 2, TOKEN_NIL);
		case 'o': return check_keyword(s, 1, "r", 1, TOKEN_OR);
		case 'p': return check_keyword(s, 1, "rint", 4, TOKEN_PRINT);
		case 'r': return check_keyword(s, 1, "eturn", 5, TOKEN_RETURN);
		case 's': return check_keyword(s, 1, "uper", 4, TOKEN_SUPER);
		case 't':
			if (s->current - s->start > 1) {
				switch (s->start[1]) {
					case 'h': return check_keyword(s, 2, "is", 2, TOKEN_THIS);
					case 'r': return check_keyword(s, 2, "ue", 2, TOKEN_TRUE);
				}
			}
			break;
		case 'v': return check_keyword(s, 1, "ar", 2, TOKEN_VAR);
		case 'w': return check_keyword(s, 1, "hile", 4, TOKEN_WHILE);
	}
}

static Token scan_identifier(Scanner* scanner)
{
	char lookahead;
	while (is_alpha(peek(scanner)) || is_digit(peek(scanner)))
		advance(scanner);

	return make_token(scanner, identifier_type(scanner));
}

Token scan_token(Scanner* scanner)
{
	skip_whitespace(scanner);
	scanner->start = scanner->current;

	if (at_end(scanner)) return make_token(scanner, TOKEN_EOF);

	const char c = advance(scanner);
	if (is_alpha(c))  return scan_identifier(scanner);
	else if (is_digit(c)) return scan_number(scanner);

	switch (c) {
		case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
		case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
		case '{': return make_token(scanner, TOKEN_LEFT_BRACE);
		case '}': return make_token(scanner, TOKEN_RIGHT_BRACE);
		case ';': return make_token(scanner, TOKEN_SEMICOLON);
		case ',': return make_token(scanner, TOKEN_COMMA);
		case '.': return make_token(scanner, TOKEN_DOT);
		case '-': return make_token(scanner, TOKEN_MINUS);
		case '+': return make_token(scanner, TOKEN_PLUS);
		case '/': return make_token(scanner, TOKEN_SLASH);
		case '*': return make_token(scanner, TOKEN_STAR);
		case '!': return make_token(scanner, match(scanner, '=')
		                                     ? TOKEN_BANG_EQUAL
		                                     : TOKEN_BANG);
		case '=': return make_token(scanner, match(scanner, '=')
		                                     ? TOKEN_EQUAL_EQUAL
		                                     : TOKEN_EQUAL);
		case '<': return make_token(scanner, match(scanner, '=')
		                                     ? TOKEN_LESS_EQUAL
		                                     : TOKEN_LESS);
		case '>': return make_token(scanner, match(scanner, '=')
		                                     ? TOKEN_GREATER_EQUAL
		                                     : TOKEN_GREATER);
		case '"': return scan_string(scanner);
	}

	return error_token(scanner, "Unexpected character.");
}
