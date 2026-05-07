// window_builtins.cpp — uLisp primitives for PicoCalc text windows.

#include <Arduino.h>

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_state.h"
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
  object *rest = cdr(cdr(cdr(cdr(args))));
  if (rest != NULL) {
    uint16_t fg = checkinteger(first(rest));
    uint16_t bg = DefaultWindowBackground;
    uint16_t border = DefaultWindowBorder;
    rest = cdr(rest);
    if (rest != NULL) {
      bg = checkinteger(first(rest));
      rest = cdr(rest);
      if (rest != NULL) border = checkinteger(first(rest));
    }
    window->setColors(fg, bg, border);
  }
  return stream(WINDOWSTREAM, window->id());
}

object *fn_movewindow(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  int x = checkinteger(second(args));
  int y = checkinteger(third(args));
  if (!windowManager.move(id, x, y)) error2("invalid window");
  return first(args);
}

object *fn_resizewindow(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  int cols = checkinteger(second(args));
  int rows = checkinteger(third(args));
  if (!windowManager.resize(id, cols, rows)) error2("invalid window");
  return first(args);
}

object *fn_clearwindow(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  TextWindow *window = windowManager.textWindow(id);
  window->clear();
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

object *fn_setwindowcolors(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  uint16_t fg = checkinteger(second(args));
  uint16_t bg = checkinteger(third(args));
  uint16_t border = DefaultWindowBorder;
  object *rest = cdr(cdr(cdr(args)));
  if (rest != NULL) border = checkinteger(first(rest));
  TextWindow *window = windowManager.textWindow(id);
  window->setColors(fg, bg, border);
  return first(args);
}

object *fn_windowredraw(object *args, object *env) {
  (void) env;
  int id = checkwindow(first(args));
  TextWindow *window = windowManager.textWindow(id);
  window->render();
  return first(args);
}

object *fn_listwindows(object *args, object *env) {
  (void) args;
  (void) env;
  object *result = nil;
  for (int id = MaxTextWindows - 1; id >= 0; id--) {
    if (windowManager.textWindow(id) != nullptr) result = cons(stream(WINDOWSTREAM, id), result);
  }
  return result;
}

object *fn_windowdebug(object *args, object *env) {
  (void) env;
  if (args == NULL || first(args) == NULL) {
    return cons(number(windowManager.focused()), NULL);
  }
  int id = checkwindow(first(args));
  TextWindow *window = windowManager.textWindow(id);
  return cons(number(id),
         cons(number(window->x()),
         cons(number(window->y()),
         cons(number(window->cols()),
         cons(number(window->rows()),
         cons(window->visible() ? tee : nil,
         cons(window->dirty() ? tee : nil,
         cons(number(windowManager.focused()), NULL))))))));
}
