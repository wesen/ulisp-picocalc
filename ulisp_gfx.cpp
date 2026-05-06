
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_messages.h"
#include "ulisp_state.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_runtime.h"
#include "ulisp_streams.h"
#include "ulisp_eval.h"
#include "ulisp_builtins.h"
#include "ulisp_gfx.h"

extern TFT_eSPI tft;
extern const int COLOR_BLACK;
extern const int COLOR_WHITE;

// Graphics functions

object *sp_withgfx (object *args, object *env) {
#if defined(gfxsupport)
  object *params = checkarguments(args, 1, 1);
  object *var = first(params);
  object *pair = cons(var, stream(GFXSTREAM, 1));
  push(pair,env);
  object *forms = cdr(args);
  object *result = eval(tf_progn(forms,env), env);
  return result;
#else
  (void) args, (void) env;
  error2("not supported");
  return nil;
#endif
}

object *fn_drawpixel (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t colour = COLOR_WHITE;
  if (cddr(args) != NULL) colour = checkinteger(third(args));
  tft.drawPixel(checkinteger(first(args)), checkinteger(second(args)), colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_drawline (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[4], colour = COLOR_WHITE;
  for (int i=0; i<4; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.drawLine(params[0], params[1], params[2], params[3], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_drawrect (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[4], colour = COLOR_WHITE;
  for (int i=0; i<4; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.drawRect(params[0], params[1], params[2], params[3], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_fillrect (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[4], colour = COLOR_WHITE;
  for (int i=0; i<4; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.fillRect(params[0], params[1], params[2], params[3], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_drawcircle (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[3], colour = COLOR_WHITE;
  for (int i=0; i<3; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.drawCircle(params[0], params[1], params[2], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_fillcircle (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[3], colour = COLOR_WHITE;
  for (int i=0; i<3; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.fillCircle(params[0], params[1], params[2], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_drawroundrect (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[5], colour = COLOR_WHITE;
  for (int i=0; i<5; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.drawRoundRect(params[0], params[1], params[2], params[3], params[4], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_fillroundrect (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[5], colour = COLOR_WHITE;
  for (int i=0; i<5; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.fillRoundRect(params[0], params[1], params[2], params[3], params[4], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_drawtriangle (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[6], colour = COLOR_WHITE;
  for (int i=0; i<6; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.drawTriangle(params[0], params[1], params[2], params[3], params[4], params[5], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_filltriangle (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t params[6], colour = COLOR_WHITE;
  for (int i=0; i<6; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  if (args != NULL) colour = checkinteger(car(args));
  tft.fillTriangle(params[0], params[1], params[2], params[3], params[4], params[5], colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_drawchar (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t colour = COLOR_WHITE, bg = COLOR_BLACK, size = 1;
  object *more = cdr(cddr(args));
  if (more != NULL) {
    colour = checkinteger(car(more));
    more = cdr(more);
    if (more != NULL) {
      bg = checkinteger(car(more));
      more = cdr(more);
      if (more != NULL) size = checkinteger(car(more));
    }
  }
  tft.drawChar(checkinteger(first(args)), checkinteger(second(args)), checkchar(third(args)),
    colour, bg, size);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_setcursor (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  tft.setCursor(checkinteger(first(args)), checkinteger(second(args)));
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_settextcolor (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  if (cdr(args) != NULL) tft.setTextColor(checkinteger(first(args)), checkinteger(second(args)));
  else tft.setTextColor(checkinteger(first(args)));
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_settextsize (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  tft.setTextSize(checkinteger(first(args)));
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_settextwrap (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  tft.setTextWrap(first(args) != NULL);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_fillscreen (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  uint16_t colour = COLOR_BLACK;
  if (args != NULL) colour = checkinteger(first(args));
  tft.fillScreen(colour);
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_setrotation (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  tft.setRotation(checkinteger(first(args)));
  #else
  (void) args;
  #endif
  return nil;
}

object *fn_invertdisplay (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  tft.invertDisplay(first(args) != NULL);
  #else
  (void) args;
  #endif
  return nil;
}

