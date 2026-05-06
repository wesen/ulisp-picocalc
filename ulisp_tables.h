// ulisp_tables.h — builtin names, docs, lookup tables, and table helpers.

#ifndef ULISP_TABLES_H
#define ULISP_TABLES_H

#include "ulisp_types.h"

const tbl_entry_t *table(int n);
unsigned int tablesize(int n);
uint8_t getminmax(builtin_t name);
fn_ptr_type getminmaxandfntype(builtin_t name, uint8_t *minmax, uint8_t *fntype);
object *bsymbol(builtin_t name);
builtin_t builtin(symbol_t name);
bool builtinp(symbol_t name);
int isbuiltin(object *x, builtin_t name);
builtin_t lookupbuiltin(char *buffer);

#endif // ULISP_TABLES_H
