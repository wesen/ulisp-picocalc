#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>

#if defined(sdcardsupport)
#include <SD.h>
#endif

#define Serial Serial1

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_state.h"
#include "ulisp_print.h"
#include "ulisp_pretty.h"
#include "ulisp_streams.h"
#include "ulisp_platform.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_runtime.h"
#include "ulisp_builtins.h"
#include "ulisp_fwd_decls.h"

extern const char notanumber[];
extern const char notaninteger[];
extern const char notastring[];
extern const char notalist[];
extern const char notasymbol[];
extern const char notproper[];
extern const char toomanyargs[];
extern const char toofewargs[];
extern const char noargument[];
extern const char nostream[];
extern const char overflow[];
extern const char divisionbyzero[];
extern const char indexnegative[];
extern const char invalidarg[];
extern const char invalidkey[];
extern const char illegalclause[];
extern const char illegalfn[];
extern const char oddargs[];
extern const char indexrange[];
extern const char unknownstreamtype[];


// Characters

object *fn_char (object *args, object *env) {
  (void) env;
  object *arg = first(args);
  if (!stringp(arg)) error(notastring, arg);
  object *n = second(args);
  char c = nthchar(arg, checkinteger(n));
  if (c == 0) error(indexrange, n);
  return character(c);
}

object *fn_charcode (object *args, object *env) {
  (void) env;
  return number(checkchar(first(args)));
}

object *fn_codechar (object *args, object *env) {
  (void) env;
  return character(checkinteger(first(args)));
}

object *fn_characterp (object *args, object *env) {
  (void) env;
  return characterp(first(args)) ? tee : nil;
}

// Strings

object *fn_stringp (object *args, object *env) {
  (void) env;
  return stringp(first(args)) ? tee : nil;
}

object *fn_stringeq (object *args, object *env) {
  (void) env;
  int m = stringcompare(args, false, false, true);
  return m == -1 ? nil : tee;
}

object *fn_stringless (object *args, object *env) {
  (void) env;
  int m = stringcompare(args, true, false, false);
  return m == -1 ? nil : number(m);
}

object *fn_stringgreater (object *args, object *env) {
  (void) env;
  int m = stringcompare(args, false, true, false);
  return m == -1 ? nil : number(m);
}

object *fn_stringnoteq (object *args, object *env) {
  (void) env;
  int m = stringcompare(args, true, true, false);
  return m == -1 ? nil : number(m);
}

object *fn_stringlesseq (object *args, object *env) {
  (void) env;
  int m = stringcompare(args, true, false, true);
  return m == -1 ? nil : number(m);
}

object *fn_stringgreatereq (object *args, object *env) {
  (void) env;
  int m = stringcompare(args, false, true, true);
  return m == -1 ? nil : number(m);
}

object *fn_sort (object *args, object *env) {
  object *arg = first(args);
  if (!listp(arg)) error(notalist, arg);
  if (arg == NULL) return nil;
  object *list = cons(nil, arg);
  protect(list);
  object *predicate = second(args);
  object *compare = cons(NULL, cons(NULL, NULL));
  protect(compare);
  object *ptr = cdr(list);
  while (cdr(ptr) != NULL) {
    object *go = list;
    while (go != ptr) {
      car(compare) = car(cdr(ptr));
      car(cdr(compare)) = car(cdr(go));
      if (apply(predicate, compare, env)) break;
      go = cdr(go);
    }
    if (go != ptr) {
      object *obj = cdr(ptr);
      cdr(ptr) = cdr(obj);
      cdr(obj) = cdr(go);
      cdr(go) = obj;
    } else ptr = cdr(ptr);
  }
  unprotect(); unprotect();
  return cdr(list);
}

object *fn_stringfn (object *args, object *env) {
  return fn_princtostring(args, env);
}

object *fn_concatenate (object *args, object *env) {
  (void) env;
  object *arg = first(args);
  if (builtin(arg->name) != STRINGFN) error2("only supports strings");
  args = cdr(args);
  object *result = newstring();
  object *tail = result;
  while (args != NULL) {
    object *obj = checkstring(first(args));
    obj = cdr(obj);
    while (obj != NULL) {
      int quad = obj->chars;
      while (quad != 0) {
         char ch = quad>>((sizeof(int)-1)*8) & 0xFF;
         buildstring(ch, &tail);
         quad = quad<<8;
      }
      obj = car(obj);
    }
    args = cdr(args);
  }
  return result;
}

object *fn_subseq (object *args, object *env) {
  (void) env;
  object *arg = first(args);
  int start = checkinteger(second(args)), end;
  if (start < 0) error(indexnegative, second(args));
  args = cddr(args);
  if (listp(arg)) {
    int length = listlength(arg);
    if (args != NULL) end = checkinteger(car(args)); else end = length;
    if (start > end || end > length) error2(indexrange);
    object *result = cons(NULL, NULL);
    object *ptr = result;
    for (int x = 0; x < end; x++) {
      if (x >= start) { cdr(ptr) = cons(car(arg), NULL); ptr = cdr(ptr); }
      arg = cdr(arg);
    }
    return cdr(result);
  } else if (stringp(arg)) {
    int length = stringlength(arg);
    if (args != NULL) end = checkinteger(car(args)); else end = length;
    if (start > end || end > length) error2(indexrange);
    object *result = newstring();
    object *tail = result;
    for (int i=start; i<end; i++) {
      char ch = nthchar(arg, i);
      buildstring(ch, &tail);
    }
    return result;
  } else error2("argument is not a list or string");
  return nil;
}

object *fn_search (object *args, object *env) {
  (void) env;
  object *pattern = first(args);
  object *target = second(args);
  if (pattern == NULL) return number(0);
  else if (target == NULL) return nil;
  
  else if (listp(pattern) && listp(target)) {
    object *test = testargument(cddr(args));
    int l = listlength(target);
    int m = listlength(pattern);
    for (int i = 0; i <= l-m; i++) {
      object *target1 = target;
      while (pattern != NULL && apply(test, cons(car(target1), cons(car(pattern), NULL)), env) != NULL) {
        pattern = cdr(pattern);
        target1 = cdr(target1);
      }
      if (pattern == NULL) return number(i);
      pattern = first(args); target = cdr(target);
    }
    return nil;

  } else if (stringp(pattern) && stringp(target)) {
    if (cddr(args) != NULL) error2("keyword argument not supported for strings");
    int l = stringlength(target);
    int m = stringlength(pattern);
    for (int i = 0; i <= l-m; i++) {
      int j = 0;
      while (j < m && nthchar(target, i+j) == nthchar(pattern, j)) j++;
      if (j == m) return number(i);
    }
    return nil;
  } else error2("arguments are not both lists or strings");
  return nil;
}

object *fn_readfromstring (object *args, object *env) {
  (void) env;
  object *arg = checkstring(first(args));
  if (stringlength(arg) == 0) error2("zero length string");
  GlobalString = arg;
  GlobalStringIndex = 0;
  object *val = readmain(gstr);
  return val;
}

object *fn_princtostring (object *args, object *env) {
  (void) env;
  return princtostring(first(args));
}

object *fn_prin1tostring (object *args, object *env) {
  (void) env;
  object *arg = first(args);
  object *obj = startstring();
  printobject(arg, pstr);
  return obj;
}

// Bitwise operators

object *fn_logand (object *args, object *env) {
  (void) env;
  int result = -1;
  while (args != NULL) {
    result = result & checkinteger(first(args));
    args = cdr(args);
  }
  return number(result);
}

object *fn_logior (object *args, object *env) {
  (void) env;
  int result = 0;
  while (args != NULL) {
    result = result | checkinteger(first(args));
    args = cdr(args);
  }
  return number(result);
}

object *fn_logxor (object *args, object *env) {
  (void) env;
  int result = 0;
  while (args != NULL) {
    result = result ^ checkinteger(first(args));
    args = cdr(args);
  }
  return number(result);
}

object *fn_lognot (object *args, object *env) {
  (void) env;
  int result = checkinteger(car(args));
  return number(~result);
}

object *fn_ash (object *args, object *env) {
  (void) env;
  int value = checkinteger(first(args));
  int count = checkinteger(second(args));
  if (count >= 0) return number(value << count);
  else return number(value >> abs(count));
}

object *fn_logbitp (object *args, object *env) {
  (void) env;
  int index = checkinteger(first(args));
  int value = checkinteger(second(args));
  return (bitRead(value, index) == 1) ? tee : nil;
}

