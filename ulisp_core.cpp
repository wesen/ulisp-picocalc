#include <Arduino.h>
/* uLisp PicoCalc Release 4.8f - www.ulisp.com
   David Johnson-Davies - www.technoblogy.com - 14th October 2025

   Licensed under the MIT license: https://opensource.org/licenses/MIT
*/

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
#include <TFT_eSPI.h>
#include <PCKeyboard.h>
PCKeyboard pc_kbd;
#define Serial Serial1     // Comment out to use the Raspberry Pi Pico micro USB port
TFT_eSPI tft = TFT_eSPI(320,320);
#include "repl_window.h"

#include "ulisp_types.h"
#include "ulisp_messages.h"
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
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_entry.h"

// Global state moved to ulisp_state.cpp.

// Error handling moved to ulisp_error.cpp.

// Shared text and PicoCalc constants moved to ulisp_messages.cpp.

// Memory management moved to ulisp_memory.cpp.

// Persistence moved to ulisp_persistence.cpp.

// Runtime helpers moved to ulisp_runtime*.cpp.

// Platform I2C, pin checks, tone, and sleep helpers moved to ulisp_platform.cpp.

// Pretty printer and editor helpers moved to ulisp_pretty.cpp.

// Core/special/list/arithmetic/system builtins moved to ulisp_builtins*.cpp.

// Arduino/Pico primitive builtins moved to ulisp_arduino.cpp.

// Graphics builtins moved to ulisp_gfx.cpp.

// PicoCalc-specific extras moved to ulisp_picocalc.cpp.

// Builtin names, docs, and lookup tables moved to ulisp_tables.cpp.

// Main evaluator moved to ulisp_eval.cpp.

// Print functions moved to ulisp_print.cpp.

// Legacy PicoCalc terminal and keyboard helpers moved to ulisp_terminal.cpp.

// Reader and serial/PicoCalc input loop moved to ulisp_reader.cpp.

// Entry points moved to ulisp_entry.cpp.
