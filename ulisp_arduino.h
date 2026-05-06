// ulisp_arduino.h — Arduino/Pico primitive builtins.

#ifndef ULISP_ARDUINO_H
#define ULISP_ARDUINO_H

#include "ulisp_types.h"

object *fn_pinmode(object *args, object *env);
object *fn_digitalread(object *args, object *env);
object *fn_digitalwrite(object *args, object *env);
object *fn_analogread(object *args, object *env);
object *fn_analogreference(object *args, object *env);
object *fn_analogreadresolution(object *args, object *env);
object *fn_analogwrite(object *args, object *env);
object *fn_analogwriteresolution(object *args, object *env);
object *fn_delay(object *args, object *env);
object *fn_millis(object *args, object *env);
object *fn_sleep(object *args, object *env);
object *fn_note(object *args, object *env);
object *fn_register(object *args, object *env);

#endif // ULISP_ARDUINO_H
