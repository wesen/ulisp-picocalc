// ulisp_entry.cpp — Arduino entry points and top-level REPL loop.
//
// This file is the next split after the CMake bridge: setup(), loop(), and the
// interactive REPL are separated from the large interpreter implementation in
// ulisp_core.cpp.  It deliberately keeps Arduino-Pico APIs for now.

#include <Arduino.h>
#include <setjmp.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>

#include "repl_window.h"

// Keep these compile-time feature switches in sync with ulisp_core.cpp while
// the remaining monolithic core is being split into shared headers.
#define printfreespace
#define serialmonitor
#define sdcardsupport
#define gfxsupport
#define assemblerlist

#if defined(sdcardsupport)
#define SDCARD_SS_PIN 17
#endif

#define nil NULL
#define push(x, y)         ((y) = cons((x),(y)))
#define pop(y)             ((y) = cdr(y))
#define protect(y)         push((y), GCStack)
#define unprotect()        pop(GCStack)
#define setflag(x)         (Flags = Flags | 1<<(x))
#define clrflag(x)         (Flags = Flags & ~(1<<(x)))
#define tstflag(x)         (Flags & 1<<(x))

constexpr int TRACEMAX = 3;

typedef uint32_t symbol_t;
typedef uint32_t builtin_t;
typedef uint16_t flags_t;

typedef struct sobject object;
typedef int (*gfun_t)();
typedef void (*pfun_t)(char);

enum builtin_subset : builtin_t {
  NIL = 0,
  TEE = 1,
  COLONA = 8,
  COLONB = 9,
  COLONC = 10,
};

enum flag_subset {
  PRINTREADABLY,
  RETURNFLAG,
  ESCAPE,
  EXITEDITOR,
  LIBRARYLOADED,
  NOESC,
  NOECHO,
  MUFFLEERRORS,
  BACKTRACE,
};

struct sobject {
  union {
    struct { sobject *car; sobject *cdr; };
    struct {
      unsigned int type;
      union {
        symbol_t name;
        int integer;
        uint32_t chars;
        float single_float;
      };
    };
  };
};

#define car(x) (((object *) (x))->car)
#define cdr(x) (((object *) (x))->cdr)

extern jmp_buf toplevel_handler;
extern jmp_buf *handler;
extern unsigned int Freespace;
extern unsigned int TraceDepth[TRACEMAX];
extern builtin_t Context;
extern uint8_t TraceStart;
extern uint8_t TraceTop;
extern object *GlobalEnv;
extern object *GCStack;
extern uint8_t BreakLevel;
extern flags_t Flags;
extern object *tee;

#if defined(sdcardsupport)
extern File SDpfile;
extern File SDgfile;
#endif

extern TFT_eSPI tft;

object *bsymbol(builtin_t name);
object *cons(object *arg1, object *arg2);
object *eval(object *form, object *env);
object *readmain(gfun_t gfun);
object *symbol(symbol_t name);

void autorunimage();
void error2(const char *string);
void gc(object *form, object *env);
void initkybd();
void initworkspace();
void initsleep();
void loadfromlibrary(object *env);
void pfstring(const char *s, pfun_t pfun);
void pint(int i, pfun_t pfun);
void pfl(pfun_t pfun);
void pln(pfun_t pfun);
void printbacktrace();
void printobject(object *form, pfun_t pfun);
void pserial(char c);
void gserial_flush();
int gserial();
void ulisperror();

void initenv () {
  GlobalEnv = NULL;
  tee = bsymbol(TEE);
}

void initgfx () {
  #if defined(gfxsupport)
  tft.init();
  tft.writecommand(TFT_DISPOFF);
  tft.invertDisplay(1);
  tft.fillScreen(TFT_BLACK);
  tft.writecommand(TFT_DISPON);
  #endif
}

void setup () {
  Serial1.begin(9600);
  int start = millis();
  while ((millis() - start) < 5000) { if (Serial1) break; }
  #if defined(sdcardsupport)
  pinMode(SDCARD_SS_PIN, OUTPUT);
  digitalWrite(SDCARD_SS_PIN,1);
  #endif
  initworkspace();
  initenv();
  initsleep();
  initgfx();
  ReplWindowInit();
  initkybd();
  pfstring(PSTR("uLisp 4.8f "), pserial); pln(pserial);
}

void repl (object *env) {
  for (;;) {
    randomSeed(micros());
    #if defined(printfreespace)
    if (!tstflag(NOECHO)) gc(NULL, env);
    pint(Freespace+1, pserial);
    #endif
    if (BreakLevel) {
      pfstring(" : ", pserial);
      pint(BreakLevel, pserial);
    }
    ReplSetPromptStyle();
    pserial('>'); pserial(' ');
    ReplSetOutputStyle();
    Context = NIL;
    object *line = readmain(gserial);
    #if defined(CPU_NRF52840)
    Serial1.flush();
    #endif
    if (BreakLevel) {
      if (line == nil || line == bsymbol(COLONC)) {
        pln(pserial); return;
      } else if (line == bsymbol(COLONA)) {
        pln(pserial); pln(pserial);
        GCStack = NULL;
        longjmp(*handler, 1);
      } else if (line == bsymbol(COLONB)) {
        pln(pserial); printbacktrace();
        line = bsymbol(2); // NOTHING
      }
    }
    if (line == (object *)2) error2("unmatched right bracket"); // KET token
    protect(line);
    pfl(pserial);
    line = eval(line, env);
    pfl(pserial);
    ReplSetOutputStyle();
    printobject(line, pserial);
    unprotect();
    pfl(pserial);
    ReplRenderIfDirty();
  }
}

void loop () {
  if (!setjmp(toplevel_handler)) {
    #if defined(resetautorun)
    volatile int autorun = 12;
    #else
    volatile int autorun = 13;
    #endif
    if (autorun == 12) autorunimage();
  }
  ulisperror();
  repl(NULL);
}

void ulisperror () {
  delay(100); while (Serial1.available()) Serial1.read();
  clrflag(NOESC); BreakLevel = 0; TraceStart = 0; TraceTop = 0;
  for (int i=0; i<TRACEMAX; i++) TraceDepth[i] = 0;
  #if defined(sdcardsupport)
  SDpfile.close(); SDgfile.close();
  #endif
  #if defined(lisplibrary)
  if (!tstflag(LIBRARYLOADED)) { setflag(LIBRARYLOADED); loadfromlibrary(NULL); }
  #endif
}
