
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_messages.h"
#include "ulisp_state.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_runtime.h"
#include "ulisp_print.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"

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


// Association lists

object *testargument (object *args) {
  object *test = bsymbol(EQ);
  if (args != NULL) {
    if (cdr(args) == NULL) error2("unpaired keyword");
    if ((isbuiltin(first(args), TEST))) test = second(args);
    else error("unsupported keyword", first(args));
  }
  return test;
}

object *delassoc (object *key, object **alist) {
  object *list = *alist;
  object *prev = NULL;
  while (list != NULL) {
    object *pair = first(list);
    if (eq(key,car(pair))) {
      if (prev == NULL) *alist = cdr(list);
      else cdr(prev) = cdr(list);
      return key;
    }
    prev = list;
    list = cdr(list);
  }
  return nil;
}

// Array utilities

int nextpower2 (int n) {
  n--; n |= n >> 1; n |= n >> 2; n |= n >> 4;
  n |= n >> 8; n |= n >> 16; n++;
  return n<2 ? 2 : n;
}

object *buildarray (int n, int s, object *def) {
  int s2 = s>>1;
  if (s2 == 1) {
    if (n == 2) return cons(def, def);
    else if (n == 1) return cons(def, NULL);
    else return NULL;
  } else if (n >= s2) return cons(buildarray(s2, s2, def), buildarray(n - s2, s2, def));
  else return cons(buildarray(n, s2, def), nil);
}

object *makearray (object *dims, object *def, bool bitp) {
  int size = 1;
  object *dimensions = dims;
  while (dims != NULL) {
    int d = car(dims)->integer;
    if (d < 0) error2("dimension can't be negative");
    size = size * d;
    dims = cdr(dims);
  }
  // Bit array identified by making first dimension negative
  if (bitp) {
    size = (size + sizeof(int)*8 - 1)/(sizeof(int)*8);
    car(dimensions) = number(-(car(dimensions)->integer));
  }
  object *ptr = myalloc();
  ptr->type = ARRAY;
  object *tree = nil;
  if (size != 0) tree = buildarray(size, nextpower2(size), def);
  ptr->cdr = cons(tree, dimensions);
  return ptr;
}

object **arrayref (object *array, int index, int size) {
  int mask = nextpower2(size)>>1;
  object **p = &car(cdr(array));
  while (mask) {
    if ((index & mask) == 0) p = &(car(*p)); else p = &(cdr(*p));
    mask = mask>>1;
  }
  return p;
}

object **getarray (object *array, object *subs, object *env, int *bit) {
  int index = 0, size = 1, s;
  *bit = -1;
  bool bitp = false;
  object *dims = cddr(array);
  while (dims != NULL && subs != NULL) {
    int d = car(dims)->integer;
    if (d < 0) { d = -d; bitp = true; }
    if (env) s = checkinteger(eval(car(subs), env)); else s = checkinteger(car(subs));
    if (s < 0 || s >= d) error("subscript out of range", car(subs));
    size = size * d;
    index = index * d + s;
    dims = cdr(dims); subs = cdr(subs);
  }
  if (dims != NULL) error2("too few subscripts");
  if (subs != NULL) error2("too many subscripts");
  if (bitp) {
    size = (size + sizeof(int)*8 - 1)/(sizeof(int)*8);
    *bit = index & (sizeof(int)==4 ? 0x1F : 0x0F);
    index = index>>(sizeof(int)==4 ? 5 : 4);
  }
  return arrayref(array, index, size);
}

void rslice (object *array, int size, int slice, object *dims, object *args) {
  int d = first(dims)->integer;
  for (int i = 0; i < d; i++) {
    int index = slice * d + i;
    if (!consp(args)) error2("initial contents don't match array type");
    if (cdr(dims) == NULL) {
      object **p = arrayref(array, index, size);
      *p = car(args);
    } else rslice(array, size, index, cdr(dims), car(args));
    args = cdr(args);
  }
}

object *readarray (int d, object *args) {
  object *list = args;
  object *dims = NULL; object *head = NULL;
  int size = 1;
  for (int i = 0; i < d; i++) {
    if (!listp(list)) error2("initial contents don't match array type");
    int l = listlength(list);
    if (dims == NULL) { dims = cons(number(l), NULL); head = dims; }
    else { cdr(dims) = cons(number(l), NULL); dims = cdr(dims); }
    size = size * l;
    if (list != NULL) list = car(list);
  }
  object *array = makearray(head, NULL, false);
  rslice(array, size, 0, head, args);
  return array;
}

object *readbitarray (gfun_t gfun) {
  char ch = gfun();
  object *head = NULL;
  object *tail = NULL;
  while (!issp(ch) && !isbr(ch)) {
    if (ch != '0' && ch != '1') error2("illegal character in bit array");
    object *cell = cons(number(ch - '0'), NULL);
    if (head == NULL) head = cell;
    else tail->cdr = cell;
    tail = cell;
    ch = gfun();
  }
  LastChar = ch;
  int size = listlength(head);
  object *array = makearray(cons(number(size), NULL), number(0), true);
  size = (size + sizeof(int)*8 - 1)/(sizeof(int)*8);
  int index = 0;
  while (head != NULL) {
    object **loc = arrayref(array, index>>(sizeof(int)==4 ? 5 : 4), size);
    int bit = index & (sizeof(int)==4 ? 0x1F : 0x0F);
    *loc = number((((*loc)->integer) & ~(1<<bit)) | (car(head)->integer)<<bit);
    index++;
    head = cdr(head);
  }
  return array;
}

void pslice (object *array, int size, int slice, object *dims, pfun_t pfun, bool bitp) {
  bool spaces = true;
  if (slice == -1) { spaces = false; slice = 0; }
  int d = first(dims)->integer;
  if (d < 0) d = -d;
  for (int i = 0; i < d; i++) {
    if (i && spaces) pfun(' ');
    int index = slice * d + i;
    if (cdr(dims) == NULL) {
      if (bitp) pint(((*arrayref(array, index>>(sizeof(int)==4 ? 5 : 4), size))->integer)>>
        (index & (sizeof(int)==4 ? 0x1F : 0x0F)) & 1, pfun);
      else printobject(*arrayref(array, index, size), pfun);
    } else { pfun('('); pslice(array, size, index, cdr(dims), pfun, bitp); pfun(')'); }
    testescape();
  }
}

void printarray (object *array, pfun_t pfun) {
  object *dimensions = cddr(array);
  object *dims = dimensions;
  bool bitp = false;
  int size = 1, n = 0;
  while (dims != NULL) {
    int d = car(dims)->integer;
    if (d < 0) { bitp = true; d = -d; }
    size = size * d;
    dims = cdr(dims); n++;
  }
  if (bitp) size = (size + sizeof(int)*8 - 1)/(sizeof(int)*8);
  pfun('#');
  if (n == 1 && bitp) { pfun('*'); pslice(array, size, -1, dimensions, pfun, bitp); }
  else {
    if (n > 1) { pint(n, pfun); pfun('A'); }
    pfun('('); pslice(array, size, 0, dimensions, pfun, bitp); pfun(')');
  }
}

// String utilities

void indent (uint8_t spaces, char ch, pfun_t pfun) {
  for (uint8_t i=0; i<spaces; i++) pfun(ch);
}

object *startstring () {
  object *string = newstring();
  GlobalString = string;
  GlobalStringTail = string;
  return string;
}

object *princtostring (object *arg) {
  object *obj = startstring();
  prin1object(arg, pstr);
  return obj;
}

void buildstring (char ch, object** tail) {
  object* cell;
  if (cdr(*tail) == NULL) {
    cell = myalloc(); cdr(*tail) = cell;
  } else if (((*tail)->chars & 0xFFFFFF) == 0) {
    (*tail)->chars |= ch<<16; return;
  } else if (((*tail)->chars & 0xFFFF) == 0) {
    (*tail)->chars |= ch<<8; return;
  } else if (((*tail)->chars & 0xFF) == 0) {
    (*tail)->chars |= ch; return;
  } else {
    cell = myalloc(); car(*tail) = cell;
  }
  car(cell) = NULL; cell->chars = ch<<24; *tail = cell;
}

object *copystring (object *arg) {
  object *obj = newstring();
  object *ptr = obj;
  arg = cdr(arg);
  while (arg != NULL) {
    object *cell =  myalloc(); car(cell) = NULL;
    if (cdr(obj) == NULL) cdr(obj) = cell; else car(ptr) = cell;
    ptr = cell;
    ptr->chars = arg->chars;
    arg = car(arg);
  }
  return obj;
}

object *readstring (uint8_t delim, bool esc, gfun_t gfun) {
  object *obj = newstring();
  object *tail = obj;
  int ch = gfun();
  if (ch == -1) return nil;
  while ((ch != delim) && (ch != -1)) {
    if (esc && ch == '\\') ch = gfun();
    buildstring(ch, &tail);
    ch = gfun();
  }
  return obj;
}

int stringlength (object *form) {
  int length = 0;
  form = cdr(form);
  while (form != NULL) {
    int chars = form->chars;
    for (int i=(sizeof(int)-1)*8; i>=0; i=i-8) {
      if (chars>>i & 0xFF) length++;
    }
    form = car(form);
  }
  return length;
}

object **getcharplace (object *string, int n, int *shift) {
  object **arg = &cdr(string);
  int top;
  if (sizeof(int) == 4) { top = n>>2; *shift = 3 - (n&3); }
  else { top = n>>1; *shift = 1 - (n&1); }
  *shift = - (*shift + 2);
  for (int i=0; i<top; i++) {
    if (*arg == NULL) break;
    arg = &car(*arg);
  }
  return arg;
}

uint8_t nthchar (object *string, int n) {
  int shift;
  object **arg = getcharplace(string, n, &shift);
  if (*arg == NULL) return 0;
  return (((*arg)->chars)>>((-shift-2)<<3)) & 0xFF;
}

int gstr () {
  char c = nthchar(GlobalString, GlobalStringIndex++);
  if (c != 0) return c;
  return '\n'; // -1?
}

void pstr (char c) {
  buildstring(c, &GlobalStringTail);
}

object *lispstring (char *s) {
  object *obj = newstring();
  object *tail = obj;
  while(1) {
    char ch = *s++;
    if (ch == 0) break;
    if (ch == '\\') ch = *s++;
    buildstring(ch, &tail);
  }
  return obj;
}

int stringcompare (object *args, bool lt, bool gt, bool eq) {
  object *arg1 = checkstring(first(args));
  object *arg2 = checkstring(second(args));
  arg1 = cdr(arg1); arg2 = cdr(arg2);
  int m = 0; chars_t a = 0, b = 0;
  while ((arg1 != NULL) || (arg2 != NULL)) {
    if (arg1 == NULL) return lt ? m : -1;
    if (arg2 == NULL) return gt ? m : -1;
    a = arg1->chars; b = arg2->chars;
    if (a < b) { if (lt) { m = m + sizeof(int); while (a != b) { m--; a = a >> 8; b = b >> 8; } return m; } else return -1; }
    if (a > b) { if (gt) { m = m + sizeof(int); while (a != b) { m--; a = a >> 8; b = b >> 8; } return m; } else return -1; }
    arg1 = car(arg1); arg2 = car(arg2);
    m = m + sizeof(int);
  }
  if (eq) { m = m - sizeof(int); while (a != 0) { m++; a = a << 8;} return m;} else return -1;
}

object *documentation (object *arg, object *env) {
  if (arg == NULL) return nil;
  if (!symbolp(arg)) error(notasymbol, arg);
  object *pair = findpair(arg, env);
  if (pair != NULL) {
    object *val = cdr(pair);
    if (listp(val) && first(val)->name == sym(LAMBDA) && cdr(val) != NULL && cddr(val) != NULL) {
      if (stringp(third(val))) return third(val);
    }
  }
  symbol_t docname = arg->name;
  if (!builtinp(docname)) return nil;
  char *docstring = lookupdoc(builtin(docname));
  if (docstring == NULL) return nil;
  object *obj = startstring();
  pfstring(docstring, pstr);
  return obj;
}

object *apropos (object *arg, bool print) {
  char buf[17], buf2[33];
  char *part = cstring(princtostring(arg), buf, 17);
  object *result = cons(NULL, NULL);
  object *ptr = result;
  // User-defined?
  object *globals = GlobalEnv;
  while (globals != NULL) {
    object *pair = first(globals);
    object *var = car(pair);
    object *val = cdr(pair);
    char *full = cstring(princtostring(var), buf2, 33);
    if (strstr(full, part) != NULL) {
      if (print) {
        printsymbol(var, pserial); pserial(' '); pserial('(');
        if (consp(val) && isbuiltin(car(val), LAMBDA)) pfstring("user function", pserial);
        else if (consp(val) && car(val)->type == CODE) pfstring("code", pserial);
        else pfstring("user symbol", pserial);
        pserial(')'); pln(pserial);
      } else {
        cdr(ptr) = cons(var, NULL); ptr = cdr(ptr);
      }
    }
    globals = cdr(globals);
    testescape();
  }
  // Built-in?
  int entries = tablesize(0) + tablesize(1);
  for (int i = 0; i < entries; i++) {
    if (findsubstring(part, (builtin_t)i)) {
      if (print) {
        uint8_t fntype = fntype(i);
        pbuiltin((builtin_t)i, pserial); pserial(' '); pserial('(');
        if (fntype == FUNCTIONS) pfstring("function", pserial);
        else if (fntype == SPECIAL_FORMS || fntype == TAIL_FORMS) pfstring("special form", pserial);
        else pfstring("symbol/keyword", pserial);
        pserial(')'); pln(pserial);
      } else {
        cdr(ptr) = cons(bsymbol(i), NULL); ptr = cdr(ptr);
      }
    }
    testescape();
  }
  return cdr(result);
}

char *cstring (object *form, char *buffer, int buflen) {
  form = cdr(checkstring(form));
  int index = 0;
  while (form != NULL) {
    int chars = form->integer;
    for (int i=(sizeof(int)-1)*8; i>=0; i=i-8) {
      char ch = chars>>i & 0xFF;
      if (ch) {
        if (index >= buflen-1) error2("no room for string");
        buffer[index++] = ch;
      }
    }
    form = car(form);
  }
  buffer[index] = '\0';
  return buffer;
}

object *iptostring (uint32_t ip) {
  union { uint32_t data2; uint8_t u8[4]; };
  object *obj = startstring();
  data2 = ip;
  for (int i=0; i<4; i++) {
    if (i) pstr('.');
    pintbase(u8[i], 10, pstr);
  }
  return obj;
}

uint32_t ipstring (object *form) {
  form = cdr(checkstring(form));
  int p = 0;
  union { uint32_t ipaddress; uint8_t ipbytes[4]; } ;
  ipaddress = 0;
  while (form != NULL) {
    int chars = form->integer;
    for (int i=(sizeof(int)-1)*8; i>=0; i=i-8) {
      char ch = chars>>i & 0xFF;
      if (ch) {
        if (ch == '.') { p++; if (p > 3) error2("illegal IP address"); }
        else ipbytes[p] = (ipbytes[p] * 10) + ch - '0';
      }
    }
    form = car(form);
  }
  return ipaddress;
}

object *value (symbol_t n, object *env) {
  while (env != NULL) {
    object *pair = car(env);
    #if !defined(BOARD_HAS_PSRAM)
    if (pair != NULL && car(pair)->name == n) return pair;
    #else
    if (pair != NULL && eqsymbol(car(pair)->name, n)) return pair;
    #endif
    env = cdr(env);
  }
  return nil;
}

object *findpair (object *var, object *env) {
  symbol_t name = var->name;
  object *pair = value(name, env);
  if (pair == NULL) pair = value(name, GlobalEnv);
  return pair;
}

bool boundp (object *var, object *env) {
  if (!symbolp(var)) error(notasymbol, var);
  return (findpair(var, env) != NULL);
}

object *findvalue (object *var, object *env) {
  object *pair = findpair(var, env);
  if (pair == NULL) error("unknown variable", var);
  return pair;
}

