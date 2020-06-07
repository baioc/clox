# clox

This is my implementation of Robert Nystrom's [Crafting Interpreters](http://www.craftinginterpreters.com/) bytecode compiler and virtual machine for the Lox language.


## Notes

It is written in standard C99 and uses core features such as [Flexible Array Members (FAMs)](https://en.wikipedia.org/wiki/Flexible_array_member), [designated struct initializers](https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html) and [Variable-Length Arrays (VLAs)](https://en.wikipedia.org/wiki/Variable-length_array).
Please note that the code here - just like in most Lox implementations - lacks internal algorithmic documentation and relies on the book for that purpose.

By "slightly more disciplined C" I simply mean there are no global variables and function-like preprocessor macros are limited to the file where they were defined (macros in headers were substituted by inline functions).


## Licensing

Since this is [what Bob chose](https://github.com/munificent/craftinginterpreters/blob/master/LICENSE) with respect to the book's code, the [Lox](lox/) implementation uses the [MIT License](LICENSE.txt).

Meanwhile, the generic data structures used throughout the codebase were implemented in a separate "Simple Generic Library" (SGL), which is released under the [Artistic License 2.0](sgl/LICENSE.txt).
It can be found in the [`sgl` folder](sgl/), which works as a completely standalone CMake project (with `ctest`s!).