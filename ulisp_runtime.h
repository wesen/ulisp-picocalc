// ulisp_runtime.h — runtime predicates, symbol helpers, math/data/env helpers.

#ifndef ULISP_RUNTIME_H
#define ULISP_RUNTIME_H

#include <Arduino.h>
#include "ulisp_types.h"

int modbacktrace(int n);
void printbacktrace();
int tracing(symbol_t name);
void trace(symbol_t name);
void untrace(symbol_t name);
bool consp(object *x);
bool listp(object *x);
object *quote(object *arg);
builtin_t builtin(symbol_t name);
symbol_t sym(builtin_t x);
int8_t toradix40(char ch);
char fromradix40(char n);
uint32_t pack40(char *buffer);
bool valid40(char *buffer);
int8_t digitvalue(char d);
int checkinteger(object *obj);
int checkbitvalue(object *obj);
float checkintfloat(object *obj);
int checkchar(object *obj);
object *checkstring(object *obj);
int isstream(object *obj);
bool builtinp(symbol_t name);
int checkkeyword(object *obj);
void checkargs(object *args);
bool eqlongsymbol(symbol_t sym1, symbol_t sym2);
bool eqsymbol(symbol_t sym1, symbol_t sym2);
bool eq(object *arg1, object *arg2);
bool equal(object *arg1, object *arg2);
int listlength(object *list);
object *checkarguments(object *args, int min, int max);
object *add_floats(object *args, float fresult);
object *subtract_floats(object *args, float fresult);
object *negate(object *arg);
object *multiply_floats(object *args, float fresult);
object *divide_floats(object *args, float fresult);
object *remmod(object *args, bool mod);
object *compare(object *args, bool lt, bool gt, bool eq);
int intpower(int base, int exp);
object *testargument(object *args);
object *delassoc(object *key, object **alist);
int nextpower2(int n);
object *buildarray(int n, int s, object *def);
object *makearray(object *dims, object *def, bool bitp);
object **arrayref(object *array, int index, int size);
object **getarray(object *array, object *subs, object *env, int *bit);
void rslice(object *array, int size, int slice, object *dims, object *args);
object *readarray(int d, object *args);
object *readbitarray(gfun_t gfun);
void pslice(object *array, int size, int slice, object *dims, pfun_t pfun, bool bitp);
void printarray(object *array, pfun_t pfun);
void indent(uint8_t spaces, char ch, pfun_t pfun);
object *startstring();
object *princtostring(object *arg);
void buildstring(char ch, object **tail);
object *copystring(object *arg);
object *readstring(uint8_t delim, bool esc, gfun_t gfun);
int stringlength(object *form);
object **getcharplace(object *string, int n, int *shift);
uint8_t nthchar(object *string, int n);
int gstr();
void pstr(char c);
object *lispstring(char *s);
int stringcompare(object *args, bool lt, bool gt, bool eq);
object *documentation(object *arg, object *env);
object *apropos(object *arg, bool print);
char *cstring(object *form, char *buffer, int buflen);
object *iptostring(uint32_t ip);
uint32_t ipstring(object *form);
object *value(symbol_t n, object *env);
object *findpair(object *var, object *env);
bool boundp(object *var, object *env);
object *findvalue(object *var, object *env);
object *closure(int tc, symbol_t name, object *function, object *args, object **env);
object *apply(object *function, object *args, object *env);
object **place(object *args, object *env, int *bit);
object *carx(object *arg);
object *cdrx(object *arg);
object *cxxxr(object *args, uint8_t pattern);
object *mapcl(object *args, object *env, bool mapl);
void mapcarfun(object *result, object **tail);
void mapcanfun(object *result, object **tail);
object *mapcarcan(object *args, object *env, mapfun_t fun, bool maplist);
object *dobody(object *args, object *env, bool star);

#endif // ULISP_RUNTIME_H
