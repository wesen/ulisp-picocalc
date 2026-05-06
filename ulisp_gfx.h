// ulisp_gfx.h — graphics builtins for TFT_eSPI-backed uLisp output.

#ifndef ULISP_GFX_H
#define ULISP_GFX_H

#include "ulisp_types.h"

object *sp_withgfx(object *args, object *env);
object *fn_drawpixel(object *args, object *env);
object *fn_drawline(object *args, object *env);
object *fn_drawrect(object *args, object *env);
object *fn_fillrect(object *args, object *env);
object *fn_drawcircle(object *args, object *env);
object *fn_fillcircle(object *args, object *env);
object *fn_drawroundrect(object *args, object *env);
object *fn_fillroundrect(object *args, object *env);
object *fn_drawtriangle(object *args, object *env);
object *fn_filltriangle(object *args, object *env);
object *fn_drawchar(object *args, object *env);
object *fn_setcursor(object *args, object *env);
object *fn_settextcolor(object *args, object *env);
object *fn_settextsize(object *args, object *env);
object *fn_settextwrap(object *args, object *env);
object *fn_fillscreen(object *args, object *env);
object *fn_setrotation(object *args, object *env);
object *fn_invertdisplay(object *args, object *env);

#endif // ULISP_GFX_H
