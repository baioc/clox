#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h> // bool
#include <stddef.h> // size_t, NULL
#include <stdint.h> // uint8_t, intptr_t

// Prints bytecode when its compilation finishes.
#define DEBUG_PRINT_CODE

// Prints opcodes and the stack during VM execution.
#define DEBUG_TRACE_EXECUTION

// Logs dynamic memory management during the Lox runtime.
#define DEBUG_DYNAMIC_MEMORY

#endif // CLOX_COMMON_H
