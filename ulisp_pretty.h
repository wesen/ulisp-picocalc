// ulisp_pretty.h — pretty printer and small tree editor helpers.

#ifndef ULISP_PRETTY_H
#define ULISP_PRETTY_H

#include "ulisp_types.h"

extern const int PPWIDTH;
extern const int GFXPPWIDTH;
extern int ppwidth;

void pcount(char c);
uint8_t atomwidth(object *obj);
uint8_t basewidth(object *obj, uint8_t base);
bool quoted(object *obj);
int subwidth(object *obj, int w);
int subwidthlist(object *form, int w);
void superprint(object *form, int lm, pfun_t pfun);
object *edit(object *fun);

#endif // ULISP_PRETTY_H
