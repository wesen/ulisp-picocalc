#include <Arduino.h>
/* uLisp PicoCalc Release 4.8f - www.ulisp.com
   David Johnson-Davies - www.technoblogy.com - 14th October 2025

   Licensed under the MIT license: https://opensource.org/licenses/MIT
*/

// Lisp Library
extern const char LispLibrary[] = "";

#include "ulisp_config.h"

// Includes

// #include "LispLibrary.h"
#include <setjmp.h>
#include <SPI.h>
#include <Wire.h>
#include <limits.h>

#if defined(sdcardsupport)
#include <SD.h>
#endif

#include <LittleFS.h>

// PicoCalc keyboard, display, and sound support
extern const int COLOR_WHITE = 0xffff;
extern const int COLOR_BLACK = 0;
#include <TFT_eSPI.h>
#include <PCKeyboard.h>
PCKeyboard pc_kbd;
#define Serial Serial1     // Comment out to use the Raspberry Pi Pico micro USB port
extern const int KEY_ESC = 0xB1;
TFT_eSPI tft = TFT_eSPI(320,320);
#include "repl_window.h"

#include "ulisp_types.h"
#include "ulisp_state.h"
#include "ulisp_streams.h"
#include "ulisp_print.h"
#include "ulisp_platform.h"
#include "ulisp_pretty.h"
#include "ulisp_terminal.h"
#include "ulisp_reader.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_gfx.h"
#include "ulisp_arduino.h"
#include "ulisp_picocalc.h"
#include "ulisp_runtime.h"
#include "ulisp_builtins.h"

#include "ulisp_fwd_decls.h"

// Global state moved to ulisp_state.cpp.

// Forward references
void pfstring (const char *s, pfun_t pfun);

// Error handling moved to ulisp_error.cpp.

// Save space as these are used multiple times
extern const char notanumber[] = "argument is not a number";
extern const char notaninteger[] = "argument is not an integer";
extern const char notastring[] = "argument is not a string";
extern const char notalist[] = "argument is not a list";
extern const char notasymbol[] = "argument is not a symbol";
extern const char notproper[] = "argument is not a proper list";
extern const char toomanyargs[] = "too many arguments";
extern const char toofewargs[] = "too few arguments";
extern const char noargument[] = "missing argument";
extern const char nostream[] = "missing stream argument";
extern const char overflow[] = "arithmetic overflow";
extern const char divisionbyzero[] = "division by zero";
extern const char indexnegative[] = "index can't be negative";
extern const char invalidarg[] = "invalid argument";
extern const char invalidkey[] = "invalid keyword";
extern const char illegalclause[] = "illegal clause";
extern const char illegalfn[] = "illegal function";
extern const char invalidpin[] = "invalid pin";
extern const char oddargs[] = "odd number of arguments";
extern const char indexrange[] = "index out of range";
extern const char canttakecar[] = "can't take car";
extern const char canttakecdr[] = "can't take cdr";
extern const char unknownstreamtype[] = "unknown stream type";

// Memory management moved to ulisp_memory.cpp.

// Persistence moved to ulisp_persistence.cpp.

// Runtime helpers moved to ulisp_runtime.cpp.

// Platform I2C, pin checks, tone, and sleep helpers moved to ulisp_platform.cpp.

// Pretty printer and editor helpers moved to ulisp_pretty.cpp.

// Core/special/list/arithmetic/system builtins moved to ulisp_builtins.cpp.

// Arduino/Pico primitive builtins moved to ulisp_arduino.cpp.

// Editor, pretty-print, format, library, docs, error, SD, and Wi-Fi builtins moved to ulisp_builtins.cpp.

// Graphics builtins moved to ulisp_gfx.cpp.

// PicoCalc-specific extras moved to ulisp_picocalc.cpp.

// Builtin names, docs, and lookup tables moved to ulisp_tables.cpp.

// Main evaluator moved to ulisp_eval.cpp.

// Print functions moved to ulisp_print.cpp.

// Legacy PicoCalc terminal and keyboard helpers moved to ulisp_terminal.cpp.

// Reader and serial/PicoCalc input loop moved to ulisp_reader.cpp.

// Entry points moved to ulisp_entry.cpp.
