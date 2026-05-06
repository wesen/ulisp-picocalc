// ulisp_reader.h — uLisp reader and serial/PicoCalc input loop.

#ifndef ULISP_READER_H
#define ULISP_READER_H

#include "ulisp_types.h"

int glibrary();
void loadfromlibrary(object *env);
void gserial_flush();
int gserial();
object *nextitem(gfun_t gfun);
object *readrest(gfun_t gfun);
int glast();
object *readmain(gfun_t gfun);
object *read(gfun_t gfun);

#endif // ULISP_READER_H
