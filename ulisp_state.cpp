// ulisp_state.cpp — storage for shared interpreter global state.

#include <Arduino.h>
#include "ulisp_state.h"

object Workspace[WORKSPACESIZE] WORDALIGNED MEMBANK;
#if defined(CODESIZE)
RAMFUNC uint8_t MyCode[CODESIZE] WORDALIGNED;
#endif

jmp_buf toplevel_handler;
jmp_buf *handler = &toplevel_handler;
unsigned int Freespace = 0;
object *Freelist;
unsigned int I2Ccount;
unsigned int TraceFn[TRACEMAX];
unsigned int TraceDepth[TRACEMAX];
builtin_t Context;
uint8_t TraceStart = 0, TraceTop = 0;
symbol_t Backtrace[BACKTRACESIZE];

object *GlobalEnv;
object *GCStack = NULL;
object *GlobalString;
object *GlobalStringTail;
int GlobalStringIndex = 0;
uint8_t PrintCount = 0;
uint8_t BreakLevel = 0;
char LastChar = 0;
char LastPrint = 0;

flags_t Flags = 1<<PRINTREADABLY; // Set by default
object *tee;
