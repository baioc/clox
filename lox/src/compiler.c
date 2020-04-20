#include "compiler.h"

#include <stdio.h>

#include "scanner.h" // Scanner, Token


void compile(const char* source)
{
	Scanner scanner;
	scanner_start(&scanner, source);
	for (int line = -1;;) {
		Token token = scan_token(&scanner);
		if (token.line != line) {
			printf("%4d ", token.line);
			line = token.line;
		} else {
			printf("   | ");
		}
		printf("%2d '%.*s'\n", token.type, token.length, token.start);
		if (token.type == TOKEN_EOF) break;
	}
}
