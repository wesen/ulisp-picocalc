// repl_window.cpp — storage for REPL window globals.
//
// The implementation functions remain inline in repl_window.h for now so the
// first split keeps behavior close to the original single-translation-unit
// sketch while giving the large state buffers exactly one definition.

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>
#include "repl_window.h"

ReplDrawState replDrawState;
ReplBackBuffer replBack;
ReplEditBuffer replEdit;
ReplRenderCell replDesired[ReplLines][ReplColumns];
ReplRenderCell replDrawn[ReplLines][ReplColumns];
bool replDrawnValid = false;
bool replUiDirty = true;
