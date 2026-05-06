// ulisp_memory.cpp — object allocation, symbol interning, features, GC, and image compaction.

#include <Arduino.h>
#include <setjmp.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <LittleFS.h>

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_state.h"
#include "ulisp_memory.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_arduino.h"
#include "ulisp_gfx.h"
#include "ulisp_picocalc.h"
#include "ulisp_messages.h"
#include "ulisp_runtime.h"
#include "ulisp_streams.h"
#include "ulisp_platform.h"
#include "ulisp_pretty.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_print.h"
#include "ulisp_terminal.h"
#include "ulisp_builtins.h"
#include "ulisp_entry.h"

// Set up workspace

void initworkspace () {
  Freelist = NULL;
  for (int i=WORKSPACESIZE-1; i>=0; i--) {
    object *obj = &Workspace[i];
    car(obj) = NULL;
    cdr(obj) = Freelist;
    Freelist = obj;
    Freespace++;
  }
}

object *myalloc () {
  if (Freespace == 0) { Context = NIL; error2("no room"); }
  object *temp = Freelist;
  Freelist = cdr(Freelist);
  Freespace--;
  return temp;
}

inline void myfree (object *obj) {
  car(obj) = NULL;
  cdr(obj) = Freelist;
  Freelist = obj;
  Freespace++;
}

// Make each type of object

object *number (int n) {
  object *ptr = myalloc();
  ptr->type = NUMBER;
  ptr->integer = n;
  return ptr;
}

object *makefloat (float f) {
  object *ptr = myalloc();
  ptr->type = FLOAT;
  ptr->single_float = f;
  return ptr;
}

object *character (uint8_t c) {
  object *ptr = myalloc();
  ptr->type = CHARACTER;
  ptr->chars = c;
  return ptr;
}

object *cons (object *arg1, object *arg2) {
  object *ptr = myalloc();
  ptr->car = arg1;
  ptr->cdr = arg2;
  return ptr;
}

object *symbol (symbol_t name) {
  object *ptr = myalloc();
  ptr->type = SYMBOL;
  ptr->name = name;
  return ptr;
}

object *bsymbol (builtin_t name) {
  return intern(twist(name+BUILTINS));
}

object *codehead (int entry) {
  object *ptr = myalloc();
  ptr->type = CODE;
  ptr->integer = entry;
  return ptr;
}

object *intern (symbol_t name) {
  #if !defined(BOARD_HAS_PSRAM)
  for (int i=0; i<WORKSPACESIZE; i++) {
    object *obj = &Workspace[i];
    if (obj->type == SYMBOL && obj->name == name) return obj;
  }
  #endif
  return symbol(name);
}

bool eqsymbols (object *obj, char *buffer) {
  object *arg = cdr(obj);
  int i = 0;
  while (!(arg == NULL && buffer[i] == 0)) {
    if (arg == NULL || buffer[i] == 0) return false;
    chars_t test = 0; int shift = 24;
    for (int j=0; j<4; j++, i++) {
      if (buffer[i] == 0) break;
      test = test | buffer[i]<<shift;
      shift = shift - 8;
    }
    if (arg->chars != test) return false;
    arg = car(arg);
  }
  return true;
}

object *internlong (char *buffer) {
  #if !defined(BOARD_HAS_PSRAM)
  for (int i=0; i<WORKSPACESIZE; i++) {
    object *obj = &Workspace[i];
    if (obj->type == SYMBOL && longsymbolp(obj) && eqsymbols(obj, buffer)) return obj;
  }
  #endif
  object *obj = lispstring(buffer);
  obj->type = SYMBOL;
  return obj;
}

object *stream (uint8_t streamtype, uint8_t address) {
  object *ptr = myalloc();
  ptr->type = STREAM;
  ptr->integer = streamtype<<8 | address;
  return ptr;
}

object *newstring () {
  object *ptr = myalloc();
  ptr->type = STRING;
  ptr->chars = 0;
  return ptr;
}

// Features

const char floatingpoint[] = ":floating-point";
const char arrays[] = ":arrays";
const char doc[] = ":documentation";
const char machinecode[] = ":machine-code";
const char errorhandling[] = ":error-handling";
const char wifi[] = ":wi-fi";
const char gfx[] = ":gfx";
const char sdcard[] = ":sd-card";
const char arm[] = ":arm";
const char riscv[] = ":risc-v";

object *features () {
  object *result = NULL;
  #if defined(__riscv)
  push(internlong((char *)riscv), result);
  #else
  push(internlong((char *)arm), result);
  #endif
  #if defined(sdcardsupport)
  push(internlong((char *)sdcard), result);
  #endif
  #if defined(gfxsupport)
  push(internlong((char *)gfx), result);
  #endif
  #if defined(ULISP_WIFI)
  push(internlong((char *)wifi), result);
  #endif
  push(internlong((char *)errorhandling), result);
  push(internlong((char *)machinecode), result);
  push(internlong((char *)doc), result);
  push(internlong((char *)arrays), result);
  push(internlong((char *)floatingpoint), result);
  return result;
}

// Garbage collection

void markobject (object *obj) {
  MARK:
  if (obj == NULL) return;
  if (marked(obj)) return;

  object* arg = car(obj);
  unsigned int type = obj->type;
  mark(obj);

  if (type >= PAIR || type == ZZERO) { // cons
    markobject(arg);
    obj = cdr(obj);
    goto MARK;
  }

  if (type == ARRAY) {
    obj = cdr(obj);
    goto MARK;
  }

  if ((type == STRING) || (type == SYMBOL && longsymbolp(obj))) {
    obj = cdr(obj);
    while (obj != NULL) {
      arg = car(obj);
      mark(obj);
      obj = arg;
    }
  }
}

void sweep () {
  Freelist = NULL;
  Freespace = 0;
  for (int i=WORKSPACESIZE-1; i>=0; i--) {
    object *obj = &Workspace[i];
    if (!marked(obj)) myfree(obj); else unmark(obj);
  }
}

void gc (object *form, object *env) {
  #if defined(printgcs)
  int start = Freespace;
  #endif
  markobject(tee);
  markobject(GlobalEnv);
  markobject(GCStack);
  markobject(form);
  markobject(env);
  sweep();
  #if defined(printgcs)
  pfl(pserial); pserial('{'); pint(Freespace - start, pserial); pserial('}');
  #endif
}

// Compact image

void movepointer (object *from, object *to) {
  uintptr_t limit = ((uintptr_t)(from) - (uintptr_t)(Workspace))/sizeof(object);
  for (uintptr_t i=0; i<=limit; i++) {
    object *obj = &Workspace[i];
    unsigned int type = (obj->type) & ~MARKBIT;
    if (marked(obj) && (type >= ARRAY || type==ZZERO || (type == SYMBOL && longsymbolp(obj)))) {
      if (car(obj) == (object *)((uintptr_t)from | MARKBIT))
        car(obj) = (object *)((uintptr_t)to | MARKBIT);
      if (cdr(obj) == from) cdr(obj) = to;
    }
  }
  // Fix strings and long symbols
  for (uintptr_t i=0; i<=limit; i++) {
    object *obj = &Workspace[i];
    if (marked(obj)) {
      unsigned int type = (obj->type) & ~MARKBIT;
      if (type == STRING || (type == SYMBOL && longsymbolp(obj))) {
        obj = cdr(obj);
        while (obj != NULL) {
          if (cdr(obj) == to) cdr(obj) = from;
          obj = (object *)((uintptr_t)(car(obj)) & ~MARKBIT);
        }
      }
    }
  }
}

uintptr_t compactimage (object **arg) {
  markobject(tee);
  markobject(GlobalEnv);
  markobject(GCStack);
  object *firstfree = Workspace;
  while (marked(firstfree)) firstfree++;
  object *obj = &Workspace[WORKSPACESIZE-1];
  while (firstfree < obj) {
    if (marked(obj)) {
      car(firstfree) = car(obj);
      cdr(firstfree) = cdr(obj);
      unmark(obj);
      movepointer(obj, firstfree);
      if (GlobalEnv == obj) GlobalEnv = firstfree;
      if (GCStack == obj) GCStack = firstfree;
      if (*arg == obj) *arg = firstfree;
      while (marked(firstfree)) firstfree++;
    }
    obj--;
  }
  sweep();
  return firstfree - Workspace;
}

