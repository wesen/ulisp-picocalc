#include <Arduino.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <Wire.h>
#include <PCKeyboard.h>

#if defined(sdcardsupport)
#include <SD.h>
#endif

#define Serial Serial1

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_state.h"
#include "ulisp_print.h"
#include "ulisp_terminal.h"
#include "repl_window.h"
#include "ulisp_reader.h"
#include "ulisp_fwd_decls.h"

extern const char LispLibrary[];
extern PCKeyboard pc_kbd;
extern const int KEY_ESC;

// Read functions

int glibrary () {
  char c = LispLibrary[GlobalStringIndex++];
  return (c != 0) ? c : -1; // -1?
}

void loadfromlibrary (object *env) {
  GlobalStringIndex = 0;
  object *line = readmain(glibrary);
  while (line != NULL) {
    protect(line);
    eval(line, env);
    unprotect();
    line = readmain(glibrary);
  }
}

void gserial_flush () {
  #if defined (serialmonitor)
  Serial.flush();
  #endif
  KybdAvailable = 0;
  WritePtr = 0;
}

int gserial () {
  #if defined (serialmonitor)
  unsigned long start = millis();
  while (true) {
    if (millis() - start > 1000) clrflag(NOECHO);
    if (Serial.available()) {
      char temp = Serial.read();
      if (temp != '\n' && !tstflag(NOECHO)) Serial.print(temp);
      return temp;
    }
    // PicoCalc keyboard -> edit buffer
    if (pc_kbd.keyCount() > 0) {
      const PCKeyboard::KeyEvent key = pc_kbd.keyEvent();
      if (key.state == PCKeyboard::StatePress) {
        uint8_t temp = static_cast<uint8_t>(key.key);
        if (temp == KEY_ESC) setflag(ESCAPE);
        else ReplProcessKey(temp);
      }
    }
    if (replEdit.committed) {
      if (replEdit.readPos < replEdit.len) return replEdit.text[replEdit.readPos++];
      ReplEditReset();
      return '\n';
    }
    if (replUiDirty) ReplRenderIfDirty();
  }
  #else
  while (true) {
    if (pc_kbd.keyCount() > 0) {
      const PCKeyboard::KeyEvent key = pc_kbd.keyEvent();
      if (key.state == PCKeyboard::StatePress) {
        uint8_t temp = static_cast<uint8_t>(key.key);
        if (temp == KEY_ESC) setflag(ESCAPE);
        else ReplProcessKey(temp);
      }
    }
    if (replEdit.committed) {
      if (replEdit.readPos < replEdit.len) return replEdit.text[replEdit.readPos++];
      ReplEditReset();
      return '\n';
    }
    if (replUiDirty) ReplRenderIfDirty();
  }
  #endif
}

object *nextitem (gfun_t gfun) {
  int ch = gfun();
  while(issp(ch)) ch = gfun();

  if (ch == ';') {
    do { ch = gfun(); if (ch == ';' || ch == '(') setflag(NOECHO); }
    while(ch != '(');
  }
  if (ch == '\n') ch = gfun();
  if (ch == -1) return nil;
  if (ch == ')') return (object *)KET;
  if (ch == '(') return (object *)BRA;
  if (ch == '\'') return (object *)QUO;

  // Parse string
  if (ch == '"') return readstring('"', true, gfun);

  // Parse symbol, character, or number
  int index = 0, base = 10, sign = 1;
  char buffer[BUFFERSIZE];
  int bufmax = BUFFERSIZE-3; // Max index
  unsigned int result = 0;
  bool isfloat = false;
  float fresult = 0.0;

  if (ch == '+') {
    buffer[index++] = ch;
    ch = gfun();
  } else if (ch == '-') {
    sign = -1;
    buffer[index++] = ch;
    ch = gfun();
  } else if (ch == '.') {
    buffer[index++] = ch;
    ch = gfun();
    if (ch == ' ') return (object *)DOT;
    isfloat = true;
  }

  // Parse reader macros
  else if (ch == '#') {
    ch = gfun();
    char ch2 = ch & ~0x20; // force to upper case
    if (ch == '\\') { // Character
      base = 0; ch = gfun();
      if (issp(ch) || isbr(ch)) return character(ch);
      else LastChar = ch;
    } else if (ch == '|') {
      do { while (gfun() != '|'); }
      while (gfun() != '#');
      return nextitem(gfun);
    } else if (ch2 == 'B') base = 2;
    else if (ch2 == 'O') base = 8;
    else if (ch2 == 'X') base = 16;
    else if (ch == '\'') return nextitem(gfun);
    else if (ch == '.') {
      setflag(NOESC);
      object *result = eval(read(gfun), NULL);
      clrflag(NOESC);
      return result;
    }
    else if (ch == '(') { LastChar = ch; return readarray(1, read(gfun)); }
    else if (ch == '*') return readbitarray(gfun);
    else if (ch >= '1' && ch <= '9' && (gfun() & ~0x20) == 'A') return readarray(ch - '0', read(gfun));
    else error2("illegal character after #");
    ch = gfun();
  }
  int valid; // 0=undecided, -1=invalid, +1=valid
  if (ch == '.') valid = 0; else if (digitvalue(ch)<base) valid = 1; else valid = -1;
  bool isexponent = false;
  int exponent = 0, esign = 1;
  float divisor = 10.0;

  while(!issp(ch) && !isbr(ch) && index < bufmax) {
    buffer[index++] = ch;
    if (base == 10 && ch == '.' && !isexponent) {
      isfloat = true;
      fresult = result;
    } else if (base == 10 && (ch == 'e' || ch == 'E')) {
      if (!isfloat) { isfloat = true; fresult = result; }
      isexponent = true;
      if (valid == 1) valid = 0; else valid = -1;
    } else if (isexponent && ch == '-') {
      esign = -esign;
    } else if (isexponent && ch == '+') {
    } else {
      int digit = digitvalue(ch);
      if (digitvalue(ch)<base && valid != -1) valid = 1; else valid = -1;
      if (isexponent) {
        exponent = exponent * 10 + digit;
      } else if (isfloat) {
        fresult = fresult + digit / divisor;
        divisor = divisor * 10.0;
      } else {
        result = result * base + digit;
      }
    }
    ch = gfun();
  }

  buffer[index] = '\0';
  if (isbr(ch)) LastChar = ch;
  if (isfloat && valid == 1) return makefloat(fresult * sign * pow(10, exponent * esign));
  else if (valid == 1) {
    if (base == 10 && result > ((unsigned int)INT_MAX+(1-sign)/2))
      return makefloat((float)result*sign);
    return number(result*sign);
  } else if (base == 0) {
    if (index == 1) return character(buffer[0]);
    const char *p = ControlCodes; char c = 0;
    while (c < 33) {
      if (strcasecmp(buffer, p) == 0) return character(c);
      p = p + strlen(p) + 1; c++;
    }
    if (index == 3) return character((buffer[0]*10+buffer[1])*10+buffer[2]-5328);
    error2("unknown character");
  }

  builtin_t x = lookupbuiltin(buffer);
  if (x == NIL) return nil;
  if (x != ENDFUNCTIONS) return bsymbol(x);
  if (index <= 6 && valid40(buffer)) return intern(twist(pack40(buffer)));
  return internlong(buffer);
}

object *readrest (gfun_t gfun) {
  object *item = nextitem(gfun);
  object *head = NULL;
  object *tail = NULL;

  while (item != (object *)KET) {
    if (item == (object *)BRA) {
      item = readrest(gfun);
    } else if (item == (object *)QUO) {
      item = cons(bsymbol(QUOTE), cons(read(gfun), NULL));
    } else if (item == (object *)DOT) {
      tail->cdr = read(gfun);
      if (readrest(gfun) != NULL) error2("malformed list");
      return head;
    } else {
      object *cell = cons(item, NULL);
      if (head == NULL) head = cell;
      else tail->cdr = cell;
      tail = cell;
      item = nextitem(gfun);
    }
  }
  return head;
}

gfun_t GFun;

int glast () {
  if (LastChar) {
    char temp = LastChar;
    LastChar = 0;
    return temp;
  }
  return GFun();
}

object *readmain (gfun_t gfun) {
  GFun = gfun;
  if (LastChar) { LastChar = 0; error2("read can only be used with one stream at a time"); }
  LastChar = 0;
  return read(glast);
}

object *read (gfun_t gfun) {
  object *item = nextitem(gfun);
  if (item == (object *)KET) error2("incomplete list");
  if (item == (object *)BRA) return readrest(gfun);
  if (item == (object *)DOT) return read(gfun);
  if (item == (object *)QUO) return cons(bsymbol(QUOTE), cons(read(gfun), NULL));
  return item;
}

