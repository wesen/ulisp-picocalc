// ulisp_error.h — error reporting and longjmp recovery helpers.

#ifndef ULISP_ERROR_H
#define ULISP_ERROR_H

#include <Arduino.h>
#include "ulisp_types.h"

void printbacktrace();
void errorsub(symbol_t fname, const char *string);
void errorsym(symbol_t fname, const char *string, object *symbol);
void errorsym2(symbol_t fname, const char *string);
void error(const char *string, object *symbol);
void error2(const char *string);
void formaterr(object *formatstr, const char *string, uint8_t p);

#endif // ULISP_ERROR_H
