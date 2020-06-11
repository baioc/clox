#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h> // bool
#include <stddef.h> // size_t, NULL
#include <stdint.h> // uint8_t, intptr_t

// Prints each token as it is lexed.
#define DEBUG_PRINT_LEXED 0

// Prints bytecode chunks when their compilation finishes.
#define DEBUG_PRINT_CODE 1

// Prints opcodes and the stack during VM execution.
#define DEBUG_TRACE_EXECUTION 0

// Prints dynamic memory management during the Lox runtime.
#define DEBUG_LOG_GC 1

// Makes the GC run on (almost) every allocation.
#define DEBUG_STRESS_GC 1

#endif // CLOX_COMMON_H
