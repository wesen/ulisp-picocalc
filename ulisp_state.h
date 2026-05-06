// ulisp_state.h — shared interpreter global state declarations.

#ifndef ULISP_STATE_H
#define ULISP_STATE_H

#include <setjmp.h>
#include "ulisp_types.h"

#define BACKTRACESIZE 8

extern object Workspace[WORKSPACESIZE];
#if defined(CODESIZE)
extern RAMFUNC uint8_t MyCode[CODESIZE] WORDALIGNED;
#endif

extern jmp_buf toplevel_handler;
extern jmp_buf *handler;
extern unsigned int Freespace;
extern object *Freelist;
extern unsigned int I2Ccount;
extern unsigned int TraceFn[TRACEMAX];
extern unsigned int TraceDepth[TRACEMAX];
extern builtin_t Context;
extern uint8_t TraceStart;
extern uint8_t TraceTop;
extern symbol_t Backtrace[BACKTRACESIZE];

extern object *GlobalEnv;
extern object *GCStack;
extern object *GlobalString;
extern object *GlobalStringTail;
extern int GlobalStringIndex;
extern uint8_t PrintCount;
extern uint8_t BreakLevel;
extern char LastChar;
extern char LastPrint;
extern flags_t Flags;
extern object *tee;

#endif // ULISP_STATE_H
