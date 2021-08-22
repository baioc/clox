# clox

**NOTE:** We use git submodules, so make sure to clone this repo with `git clone --recursive`

This is my C99 implementation of Robert Nystrom's [Crafting Interpreters](http://www.craftinginterpreters.com/) single-pass compiler and bytecode virtual machine for the [Lox programming language](https://www.craftinginterpreters.com/the-lox-language.html).


## Sample Lox script

```js
var t0 = clock();

{
	print "Hello, World!";    // => "Hello, World!"
	print ("a" + "a") + "aa"; // => "aaaa"
	print 123 == "123";       // => false
}

fun empower(operation, base) {
	fun power(x, n) {
		var y = base;
		while (n > 0) {
			y = operation(y, x);
			n = n - 1;
		}
		return y;
	}
	return power;
}
fun add(a, b) { return a + b; }
fun printfn(x) { print x; }

{
	print add(4, 3);             // => 7

	var times = empower(add, 0);
	print times(4, 3);           // => 12

	var pow = empower(times, 1);
	print pow(4, 3);             // => 64
}

{
	print add("Hello, ", "World!"); // => "Hello, World!"
	var repeat = empower(add, "");
	print repeat("a", 4);           // => "aaaa"
}

class A {
	init() {
		error("K.O."); // should never run
	}
}
class B < A {
	init() {
		print "OK";
	}
	foo(x) {
		print this;
	}
}

{
	var obj = B(); // => "OK"
	obj.foo(nil);  // => B instance
	setField(obj, "foo", printfn);
	obj.foo("ok"); // => "ok"
}

print clock() - t0;
```


## Notes

Please note that the code here - just like in most Lox implementations - is somewhat lacking in internal algorithmic documentation and relies on the book for that purpose.

In this version there are no global variables and function-like preprocessor macros are limited to the file where they are defined (macros in headers were substituted by inline functions).


## Licensing

Since this is [what Bob chose](https://github.com/munificent/craftinginterpreters/blob/master/LICENSE) with respect to the book's code, the [Lox implementation](lox/) uses the [MIT License](LICENSE.txt).

Meanwhile, the generic data structures used throughout the codebase were implemented in a separate submodule ["Unsafe Generic Library" (UGLy)](https://gitlab.com/baioc/UGLy), which has its own license.
