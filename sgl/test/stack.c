#include <sgl/stack.h>

#include <assert.h>
#include <string.h> // strlen

#include <sgl/core.h> // ARRAY_SIZE


// Checks if string has a balanced sequence of brackets <(), [], {}>.
static bool balanced(const char* string)
{
	stack_t stack;
	stack_init(&stack, 0, sizeof(char), NULL);

	const size_t length = strlen(string);
	for (int i = 0; i < length; ++i) {
		const char c = string[i];

		// if opening, push it on top of the stack
		if (c == '(' || c == '[' || c == '{') {
			stack_push(&stack, &c);
		}
		// if closing, the top of the stack must be the opening
		else if (c == ')' || c == ']' || c == '}') {
			if (stack_empty(&stack)) {
				goto FAIL;
			}
			else {
				char last_open = *(char*)stack_peek(&stack, 0);
				if (   (last_open == '(' && c != ')')
				    || (last_open == '[' && c != ']')
				    || (last_open == '{' && c != '}')
				   ) {
					goto FAIL;
				} else {
					stack_pop(&stack, &last_open);
				}
			}
		}
	}

	// success when all brackets were closed
	if (stack_empty(&stack)) {
		stack_destroy(&stack);
		return true;
	}

FAIL:
	stack_destroy(&stack);
	return false;
}

int main(int argc, const char* argv[])
{
	const char* parens[] = {
		"(foo)",
		"ab)ba",
		"0(1[2[3]2]1{2[3]2}1{2(3)2}1)0",
		"(this[is)(a)(test])({braclets}{must)(match})",
		"[[ignore](other){stuff]((...)",
	};

	const bool answers[] = {
		true,
		false,
		true,
		false,
		false,
	};

	assert(ARRAY_SIZE(parens) == ARRAY_SIZE(answers));

	for (int i = 0; i < ARRAY_SIZE(parens); ++i)
		assert(balanced(parens[i]) == answers[i]);

	return 0;
}
