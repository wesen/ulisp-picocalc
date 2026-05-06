// ulisp_picocalc.h — PicoCalc-specific builtins and helpers.

#ifndef ULISP_PICOCALC_H
#define ULISP_PICOCALC_H

#include "ulisp_types.h"

#if defined(sdcardsupport)
#include <SD.h>
#endif

char getKey();
void writeTwo(File SDfile, uint16_t word);
void writeFour(File SDfile, uint32_t word);
void savebmp(object *arg);

#endif // ULISP_PICOCALC_H
