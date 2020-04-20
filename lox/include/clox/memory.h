#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "value.h" // Obj

// Deallocates all Objs in the OBJECTS linked list.
void free_objects(Obj** objects);

#endif // CLOX_MEMORY_H
