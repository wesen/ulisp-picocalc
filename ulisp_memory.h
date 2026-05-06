// ulisp_memory.h — allocation, constructors, GC, symbol interning, and image compaction.

#ifndef ULISP_MEMORY_H
#define ULISP_MEMORY_H

#include "ulisp_types.h"

void initworkspace();
object *myalloc();
void myfree(object *obj);
object *number(int n);
object *makefloat(float f);
object *character(uint8_t c);
object *cons(object *arg1, object *arg2);
object *symbol(symbol_t name);
object *bsymbol(builtin_t name);
object *codehead(int entry);
object *intern(symbol_t name);
bool eqsymbols(object *obj, char *buffer);
object *internlong(char *buffer);
object *stream(uint8_t streamtype, uint8_t address);
object *newstring();
object *features();
void markobject(object *obj);
void sweep();
void gc(object *form, object *env);
void movepointer(object *from, object *to);
uintptr_t compactimage(object **arg);

#endif // ULISP_MEMORY_H
