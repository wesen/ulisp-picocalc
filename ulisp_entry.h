// ulisp_entry.h — Arduino sketch entry points and REPL integration.

#ifndef ULISP_ENTRY_H
#define ULISP_ENTRY_H

#include "ulisp_types.h"

void initenv();
void initgfx();
void setup();
void repl(object *env);
void loop();
void ulisperror();

#endif // ULISP_ENTRY_H
