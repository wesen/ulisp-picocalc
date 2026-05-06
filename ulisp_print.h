// ulisp_print.h — core uLisp object and scalar printing helpers.

#ifndef ULISP_PRINT_H
#define ULISP_PRINT_H

#include "ulisp_types.h"

extern const char ControlCodes[];

void pserial(char c);
void pcharacter(uint8_t c, pfun_t pfun);
void pstring(char *s, pfun_t pfun);
void plispstring(object *form, pfun_t pfun);
void plispstr(symbol_t name, pfun_t pfun);
void printstring(object *form, pfun_t pfun);
void pbuiltin(builtin_t name, pfun_t pfun);
void pradix40(symbol_t name, pfun_t pfun);
void printsymbol(object *form, pfun_t pfun);
void psymbol(symbol_t name, pfun_t pfun);
void pfstring(const char *s, pfun_t pfun);
void pint(int i, pfun_t pfun);
void pintbase(uint32_t i, uint8_t base, pfun_t pfun);
void printhex4(int i, pfun_t pfun);
void pmantissa(float f, pfun_t pfun);
void pfloat(float f, pfun_t pfun);
void pln(pfun_t pfun);
void pfl(pfun_t pfun);
void plist(object *form, pfun_t pfun);
void pstream(object *form, pfun_t pfun);
void printobject(object *form, pfun_t pfun);
void prin1object(object *form, pfun_t pfun);

#endif // ULISP_PRINT_H
