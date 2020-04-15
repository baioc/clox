#include <sgl/list.h>

#include <assert.h>
#include <string.h> // strdup

#include <sgl/core.h> // ARRAY_SIZE, freeref, bool


static int intcmp(const int* a, const int* b)
{
	return *a - *b;
}

static void list_primitives(void)
{
	const int array[] = {-6, 0, 2, 3, 6, 7, 11};
	const int n = ARRAY_SIZE(array);
	list_t numbers;

	// initially, size should be 0
	int err = list_init(&numbers, 0, sizeof(int));
	assert(!err);
	assert(list_size(&numbers) == 0);

	// reverse insert numbers in reverse order -> normal order
	for (int i = n - 1; i >= 0; --i) {
		err = list_insert(&numbers, 0, &array[i]);
		assert(!err);
	}

	// size should be the number of numbers
	assert(list_size(&numbers) == n);

	// test bsearch
	int x = 6;
	int found = list_bsearch(&numbers, &x, (int (*)(const void*, const void*))intcmp);
	assert(found == 4);
	x = -11;
	found = list_bsearch(&numbers, &x, (int (*)(const void*, const void*))intcmp);
	assert(found < 0);

	// check if the list's front is the expected value, then pop it
	for (int i = 0; i < n; ++i) {
		int num = *((int*)list_ref(&numbers, 0));
		assert(num == array[i]);
		list_remove(&numbers, 0, &num);
	}

	// after removals, size should be zero
	assert(list_size(&numbers) == 0);

	list_destroy(&numbers);
}

static void list_pointers(void)
{
	const char* array[] = {"Alyssa", "Bob", "Carlos"};
	const int n = ARRAY_SIZE(array);
	list_t names;

	// initially, size should be 0
	int err = list_init(&names, 3, sizeof(char*));
	assert(!err);
	assert(list_size(&names) == 0);

	// only the pointers are inserted into the list
	for (int i = 0; i < n; ++i) {
		const char* copy = strdup(array[i]); // must be freed
		err = list_append(&names, &copy);
		assert(!err);
	}

	// frees each pointer (they are still on the list)
	list_for_each(&names, (void (*)(void*))freeref);
	assert(list_size(&names) == n);

	// check if for_each did the right thing
	for (int i = 0; i < n; ++i) {
		const char* str = *((char**)list_ref(&names, i));
		assert(str == NULL);
	}

	list_destroy(&names);
}

// Checks if string has a balanced sequence of brackets <(), [], {}>.
bool balanced(const char* string)
{
	list_t stack;
	list_init(&stack, 0, sizeof(char));

	const size_t length = strlen(string);
	for (int i = 0; i < length; ++i) {
		const char c = string[i];

		// if opening, push it on top of the stack
		if (c == '(' || c == '[' || c == '{') {
			list_append(&stack, &c);
		}
		// if closing, the top of the stack must be the opening
		else if (c == ')' || c == ']' || c == '}') {
			if (list_empty(&stack)) {
				goto FAIL;
			}
			else {
				char last_open;
				list_pop(&stack, &last_open);

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
	if (list_empty(&stack)) {
		list_destroy(&stack);
		return true;
	}

	FAIL: list_destroy(&stack);
	return false;
}

static void list_stack(void)
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
	list_stack();
	return 0;
}
