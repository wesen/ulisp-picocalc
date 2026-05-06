#include <Arduino.h>
#include <Wire.h>

#if defined(sdcardsupport)
#include <SD.h>
#endif

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_state.h"
#include "ulisp_print.h"
#include "ulisp_pretty.h"
#include "ulisp_messages.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_runtime.h"
#include "ulisp_streams.h"
#include "ulisp_platform.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_terminal.h"
#include "ulisp_builtins.h"
#include "ulisp_arduino.h"
#include "ulisp_gfx.h"
#include "ulisp_picocalc.h"
#include "ulisp_entry.h"


// Prettyprint

const int PPINDENT = 2;
extern const int PPWIDTH = 52;
extern const int GFXPPWIDTH = 52; // 320 pixel wide screen
int ppwidth = PPWIDTH;

void pcount (char c) {
  if (c == '\n') PrintCount++;
  PrintCount++;
}

uint8_t atomwidth (object *obj) {
  PrintCount = 0;
  printobject(obj, pcount);
  return PrintCount;
}

uint8_t basewidth (object *obj, uint8_t base) {
  PrintCount = 0;
  pintbase(obj->integer, base, pcount);
  return PrintCount;
}

bool quoted (object *obj) {
  return (consp(obj) && car(obj) != NULL && car(obj)->name == sym(QUOTE) && consp(cdr(obj)) && cddr(obj) == NULL);
}

int subwidth (object *obj, int w) {
  if (atom(obj)) return w - atomwidth(obj);
  if (quoted(obj)) obj = car(cdr(obj));
  return subwidthlist(obj, w - 1);
}

int subwidthlist (object *form, int w) {
  while (form != NULL && w >= 0) {
    if (atom(form)) return w - (2 + atomwidth(form));
    w = subwidth(car(form), w - 1);
    form = cdr(form);
  }
  return w;
}

void superprint (object *form, int lm, pfun_t pfun) {
  if (atom(form)) {
    if (isbuiltin(form, NOTHING)) printsymbol(form, pfun);
    else printobject(form, pfun);
  } else if (quoted(form)) {
    pfun('\'');
    superprint(car(cdr(form)), lm + 1, pfun);
  } else {
    lm = lm + PPINDENT;
    bool fits = (subwidth(form, ppwidth - lm - PPINDENT) >= 0);
    int special = 0, extra = 0; bool separate = true;
    object *arg = car(form);
    if (symbolp(arg) && builtinp(arg->name)) {
      uint8_t minmax = getminmax(builtin(arg->name));
      if (minmax == 0327 || minmax == 0313) special = 2; // defun, setq, setf, defvar
      else if (minmax == 0317 || minmax == 0017 || minmax == 0117 || minmax == 0123) special = 1;
    }
    while (form != NULL) {
      if (atom(form)) { pfstring(" . ", pfun); printobject(form, pfun); pfun(')'); return; }
      else if (separate) { 
        pfun('(');
        separate = false;
      } else if (special) {
        pfun(' ');
        special--; 
      } else if (fits) {
        pfun(' ');
      } else { pln(pfun); indent(lm, ' ', pfun); }
      superprint(car(form), lm+extra, pfun);
      form = cdr(form);
    }
    pfun(')');
    testescape();
  }
}

object *edit (object *fun) {
  while (1) {
    if (tstflag(EXITEDITOR)) return fun;
    char c = gserial();
    if (c == 'q') setflag(EXITEDITOR);
    else if (c == 'b') return fun;
    else if (c == 'r') fun = read(gserial);
    else if (c == '\n') { pfl(pserial); superprint(fun, 0, pserial); pln(pserial); }
    else if (c == 'c') fun = cons(read(gserial), fun);
    else if (atom(fun)) pserial('!');
    else if (c == 'd') fun = cons(car(fun), edit(cdr(fun)));
    else if (c == 'a') fun = cons(edit(car(fun)), cdr(fun));
    else if (c == 'x') fun = cdr(fun);
    else pserial('?');
  }
}

