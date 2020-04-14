#include <sgl/list.h>

#include <assert.h>
#include <string.h> // strdup
#include <stdlib.h> // free
#include <stdbool.h>


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// Checks if string has a balanced sequence of brackets <(), [], {}>.
bool balanced(const char* string);


static inline void list_primitives(void)
{
	const int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
	const int n = ARRAY_SIZE(array);
	list_t numbers;

	// initially, size should be 0
	list_init_sized(&numbers, sizeof(int), 0);
	assert(list_size(&numbers) == 0);

	// array numbers are inserted as if in a reversed stack
	// numbers := [8, 7, 6, 5, 4, 3, 2, 1]
	for (int i = 0; i < n; ++i)
		list_insert_at(&numbers, &array[i], 0);

	// size should be the number of numbers
	assert(list_size(&numbers) == n);

	for (int i = n - 1; i >= 0; --i) {
		// check if the list's front is the expected value, then pop it
		int num = *((int*)list_ref(&numbers, 0));
		assert(num == array[i]);
		list_remove_from(&numbers, &num, 0);
	}

	// after removals, size should be zero
	assert(list_size(&numbers) == 0);

	list_destroy(&numbers);
}

static void strfree(void* ptr)
{
	char** str_ptr = (char**) ptr;
	free(*str_ptr);
	*str_ptr = NULL;
}

static inline void list_pointers(void)
{
	const char* array[] = {"Alyssa", "Bob", "Carlos"};
	const int n = ARRAY_SIZE(array);
	list_t names;

	// initially, size should be 0
	list_init_sized(&names, sizeof(char*), 3);
	assert(list_size(&names) == 0);

	// only the pointers are inserted into the list
	for (int i = 0; i < n; ++i) {
		const char* copy = strdup(array[i]); // must be freed
		list_insert(&names, &copy);
	}

	// frees each pointer (they are still on the list)
	list_for_each(&names, strfree);
	assert(list_size(&names) == n);

	// check if for_each did the right thing
	for (int i = 0; i < n; ++i) {
		const char* str = *((char**)list_ref(&names, i));
		assert(str == NULL);
	}

	list_destroy(&names);
}


bool balanced(const char* string)
{
	list_t stack;
	list_init(&stack, sizeof(char));

	const size_t length = strlen(string);
	for (int i = 0; i < length; ++i) {
		const char c = string[i];

		// if opening, push it on top of the stack
		if (c == '(' || c == '[' || c == '{') {
			list_insert(&stack, &c);
		}
		// if closing, the top of the stack must be the opening
		else if (c == ')' || c == ']' || c == '}') {
			if (list_size(&stack) <= 0) {
				goto FAIL;
			}
			else {
				char last_open;
				list_remove(&stack, &last_open);

				if (   (last_open == '(' && c != ')')
				    || (last_open == '[' && c != ']')
				    || (last_open == '{' && c != '}')
				   ) {
					goto FAIL;
				}
			}
		}
	}

	// success when all brackets were closed
	if (list_size(&stack) == 0) {
		list_destroy(&stack);
		return true;
	}

	FAIL: list_destroy(&stack);
	return false;
}

static inline void list_balance_check(void)
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
}

int main(int argc, const char* argv[])
{
	list_primitives();
	list_pointers();
	list_balance_check();
	return 0;
}
