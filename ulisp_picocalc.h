// ulisp_picocalc.h — PicoCalc-specific builtins and helpers.

#ifndef ULISP_PICOCALC_H
#define ULISP_PICOCALC_H

#include "ulisp_types.h"

#if defined(sdcardsupport)
#include <SD.h>
#endif

char getKey();
object *fn_getkey(object *args, object *env);
object *fn_readpixel(object *args, object *env);
void writeTwo(File SDfile, uint16_t word);
void writeFour(File SDfile, uint32_t word);
void savebmp(object *arg);
object *fn_savebmp(object *args, object *env);

#endif // ULISP_PICOCALC_H
