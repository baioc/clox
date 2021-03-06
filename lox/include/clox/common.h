#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h> // bool
#include <stddef.h> // size_t, NULL
#include <stdint.h>

// Prints each token as it is lexed.
#define DEBUG_PRINT_LEXED 0

// Prints bytecode chunks when their compilation finishes.
#define DEBUG_PRINT_CODE 0

// Prints opcodes and the stack during VM execution.
#define DEBUG_TRACE_EXECUTION 0

// Prints dynamic memory management during the Lox runtime.
#define DEBUG_LOG_GC 0

// Makes the GC run on every allocation.
#define DEBUG_STRESS_GC 0

// Initial heap size, in bytes.
#define GC_HEAP_INITIAL (1024 * 1024)

/* Whether or not to use NaN boxing to save space occupied by Lox Values.
See http://craftinginterpreters.com/optimization.html#nan-boxing for info. */
#define NAN_BOXING 1

// Whether the main VM loop should use computed gotos instead of switching.
#ifdef __GNUC__
#	define COMPUTED_GOTO 1
#else
#	define COMPUTED_GOTO 0
#endif

#endif // CLOX_COMMON_H
