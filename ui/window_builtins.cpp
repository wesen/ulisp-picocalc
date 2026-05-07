// window_builtins.cpp — uLisp primitives for PicoCalc text windows.

#include <Arduino.h>

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_runtime.h"
#include "ulisp_streams.h"
#include "window.h"

int checkwindow(object *obj) {
  int encoded = isstream(obj);
  if ((encoded >> 8) != WINDOWSTREAM) error2("not a window");
  int id = encoded & 0xFF;
  if (windowManager.textWindow(id) == nullptr) error2("invalid window");
  return id;
}

object *fn_makewindow(object *args, object *env) {
  (void) env;
  int x = checkinteger(first(args));
  int y = checkinteger(second(args));
  int cols = checkinteger(third(args));
  int rows = checkinteger(car(cdr(cdr(cdr(args)))));
  TextWindow *window = windowManager.createTextWindow(x, y, cols, rows);
  if (window == nullptr) error2("no free windows");
  window->renderIfDirty();
  return stream(WINDOWSTREAM, window->id());
}

object *fn_movewindow(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  int x = checkinteger(second(args));
  int y = checkinteger(third(args));
  if (!windowManager.move(id, x, y)) error2("invalid window");
  windowManager.renderAllDirty();
  return first(args);
}

object *fn_resizewindow(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  int cols = checkinteger(second(args));
  int rows = checkinteger(third(args));
  if (!windowManager.resize(id, cols, rows)) error2("invalid window");
  windowManager.renderAllDirty();
  return first(args);
}

object *fn_clearwindow(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  TextWindow *window = windowManager.textWindow(id);
  window->clear();
  window->renderIfDirty();
  return nil;
}

object *fn_closewindow(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  if (!windowManager.close(id)) error2("invalid window");
  return nil;
}

object *fn_focuswindow(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  if (!windowManager.focus(id)) error2("invalid window");
  return first(args);
}

object *fn_windowposition(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  TextWindow *window = windowManager.textWindow(id);
  return cons(number(window->x()), cons(number(window->y()), NULL));
}

object *fn_windowsize(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  TextWindow *window = windowManager.textWindow(id);
  return cons(number(window->cols()), cons(number(window->rows()), NULL));
}
