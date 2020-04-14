#include "value.h"

extern inline void value_print(Value value);

extern inline void value_array_init(ValueArray* array);
extern inline void value_array_destroy(ValueArray* array);
extern inline int value_array_size(const ValueArray* array);
extern inline Value value_array_get(const ValueArray* array, int index);
extern inline void value_array_write(ValueArray* array, Value value);
