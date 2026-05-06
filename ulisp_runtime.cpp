#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>

#if defined(sdcardsupport)
#include <SD.h>
#endif

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_messages.h"
#include "ulisp_state.h"
#include "ulisp_print.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_runtime.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_streams.h"
#include "ulisp_platform.h"
#include "ulisp_pretty.h"
#include "ulisp_reader.h"
#include "ulisp_terminal.h"
#include "ulisp_builtins.h"
#include "ulisp_arduino.h"
#include "ulisp_gfx.h"
#include "ulisp_picocalc.h"
#include "ulisp_entry.h"

extern const char notanumber[];
extern const char notaninteger[];
extern const char notastring[];
extern const char notalist[];
extern const char notasymbol[];
extern const char notproper[];
extern const char noargument[];
extern const char indexnegative[];
extern const char indexrange[];
extern const char unknownstreamtype[];
extern const char invalidkey[];
extern const char toofewargs[];
extern const char toomanyargs[];
extern const char divisionbyzero[];
extern const char illegalfn[];
extern const char canttakecar[];
extern const char canttakecdr[];


// Runtime helpers are split across:
// - ulisp_runtime_symbols.cpp
// - ulisp_runtime_math.cpp
// - ulisp_runtime_data.cpp
// - ulisp_runtime_env.cpp
