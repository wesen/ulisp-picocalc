// ulisp_runtime.h — core runtime helper functions for symbols, arrays, strings, closures, and mapping.

#ifndef ULISP_RUNTIME_H
#define ULISP_RUNTIME_H

#include "ulisp_types.h"

int modbacktrace(int n);
void printbacktrace();
bool consp(object *x);
bool listp(object *x);
object *quote(object *arg);
symbol_t sym(builtin_t x);
void indent(uint8_t spaces, char ch, pfun_t pfun);

#endif // ULISP_RUNTIME_H
