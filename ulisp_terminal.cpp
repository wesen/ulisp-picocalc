#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_messages.h"
#include "ulisp_state.h"
#include "ulisp_print.h"
#include "ulisp_platform.h"
#include "ulisp_terminal.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_runtime.h"
#include "ulisp_streams.h"
#include "ulisp_pretty.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_builtins.h"
#include "ulisp_arduino.h"
#include "ulisp_gfx.h"
#include "ulisp_picocalc.h"
#include "ulisp_entry.h"

extern TFT_eSPI tft;
extern PCKeyboard pc_kbd;
extern const int COLOR_BLACK;
extern const int COLOR_WHITE;
extern const int KEY_ESC;


// PicoCalc terminal and keyboard support

const int ScreenWidth = 320, ScreenHeight = 320;
const int CharWidth = 6, CharHeight = 8, Leading = 10; // Between 8 and 10
const int Columns = ScreenWidth/CharWidth;
const int TextWidth = Columns*CharWidth;
const int Lines = ScreenHeight/Leading;
const int LastColumn = Columns-1;
const int LastLine = Lines-1;
const char Cursor = 0x5f;

volatile int WritePtr = 0, ReadPtr = 0, LastWritePtr = 0;
const int KybdBufSize = Columns*Lines;
char KybdBuf[KybdBufSize], ScrollBuf[Columns][Lines];
volatile uint8_t KybdAvailable = 0;
uint8_t Scroll = 0;

// Terminal **********************************************************************************

void PlotChar (uint8_t ch, uint8_t line, uint8_t column) {
 #if defined(gfxsupport)
  uint16_t y = line*Leading;
  uint16_t x = column*CharWidth;
  ScrollBuf[column][(line+Scroll) % Lines] = ch;
  if (ch & 0x80) {
    tft.drawChar(x, y, ch & 0x7f, TFT_BLACK, TFT_GREEN, 1);
  } else {
    tft.drawChar(x, y, ch & 0x7f, TFT_WHITE, TFT_BLACK, 1);
  }
#endif
}

void ScrollDisplay () {
  #if defined(gfxsupport)
  tft.fillRect(0, ScreenHeight-Leading, ScreenWidth, Leading, TFT_BLACK);
  for (uint8_t x = 0; x < Columns; x++) {
    char c = ScrollBuf[x][Scroll];
    for (uint8_t y = 0; y < Lines-1; y++) {
      char c2 = ScrollBuf[x][(y+Scroll+1) % Lines];
      if (c != c2) {
        if (c2 & 0x80) {
          tft.drawChar(x*CharWidth, y*Leading, c2 & 0x7f, TFT_BLACK, TFT_GREEN, 1);
        } else {
          tft.drawChar(x*CharWidth, y*Leading, c2 & 0x7f, TFT_WHITE, TFT_BLACK, 1);
        }
        c = c2;
      }
    }
  }
  // Tidy up any graphics left behind on the screen
  for (uint8_t y = 0; y < Lines-1; y++) tft.fillRect(0, y*Leading+8, 320, Leading-8, TFT_BLACK);
  tft.fillRect(TextWidth, 0,  ScreenWidth-TextWidth, ScreenHeight, TFT_BLACK);
  for (int x=0; x<Columns; x++) ScrollBuf[x][Scroll] = 0;
  Scroll = (Scroll + 1) % Lines;
  #endif
}

const char VT = 11; // Vertical tab
const char BEEP = 7;
const char SHIFTRETURN = 209;

void Display (char c) {
  #if defined(gfxsupport)
  static uint8_t line = 0, column = 0;
  // These characters don't affect the cursor
  if (c == 8) {                            // Backspace
    if (column == 0) {
      line--; column = LastColumn;
    } else column--;
    return;
  }
  if (c == 9) {                            // Cursor forward
    if (column == LastColumn) {
      line++; column = 0;
    } else column++;
    return;
  }
  if ((c >= 17) && (c <= 20)) {            // Parentheses
    if (c == 17) PlotChar('(', line, column);
    else if (c == 18) PlotChar('(' | 0x80, line, column);
    else if (c == 19) PlotChar(')', line, column);
    else PlotChar(')' | 0x80, line, column);
    return;
  }
  // Hide cursor
  PlotChar(' ', line, column);
  if (c == 0x7F) {                         // DEL
    if (column == 0) {
      line--; column = LastColumn;
    } else column--;
  } else if ((c & 0x7f) >= 32) {           // Normal character
    PlotChar(c, line, column++);
    if (column > LastColumn) {
      column = 0;
      if (line == LastLine) ScrollDisplay(); else line++;
    }
  // Control characters
  } else if (c == 12) {                    // Clear display
    tft.fillScreen(COLOR_BLACK); line = 0; column = 0; Scroll = 0;
    for (int col = 0; col < Columns; col++) {
      for (int row = 0; row < Lines; row++) {
        ScrollBuf[col][row] = 0;
      }
    }
  } else if (c == '\n') {                  // Newline
    column = 0;
    if (line == LastLine) ScrollDisplay(); else line++;
  } else if (c == VT) {            // Used by Lisp Screen Editor
    column = 0; Scroll = 0; line = LastLine - 2;
  } else if (c == BEEP) { playnote(0, 0, 4); delay(250); nonote(0); } // Beep
  // Show cursor
  PlotChar(Cursor, line, column);
 #endif
}

// Keyboard **********************************************************************************

void initkybd () {
  Wire1.setSDA(6);
  Wire1.setSCL(7);
  Wire1.begin();
  Wire1.setClock(10000);
  pc_kbd.begin(0x1f,&Wire1);
}

bool reset_autocomplete = false;

void autoComplete () {
  static int bufIndex = 0, matchLen = 0, LastKeywordLenDif = 0;
  static unsigned int i = 0;
  int gap = 0;
  
  // Only update what we're matching if we're not already looking through the buffer
  if (reset_autocomplete == true) { 
    i = 0; // Reset the search
    LastKeywordLenDif = 0;
    reset_autocomplete = false;
    bufIndex = WritePtr;
    for (matchLen = 0; matchLen < 32; matchLen++) {
      int bufLoc = WritePtr - matchLen; //scan the buffer backwards away from the last character written
      if ((KybdBuf[bufLoc] == ' ') || (KybdBuf[bufLoc] == '(') || (KybdBuf[bufLoc] == '\n')) {
        if (matchLen > 0) { //if the first character is one of those then we don't have to keep looking
          // if we found these characters then go forward to the previous character
          bufIndex = bufLoc + 1;
          matchLen--;
        }
        break;
      }
      // Do this test here in case the first character in the buffer is one of the characters we test for
      else if (bufLoc == 0) { 
        bufIndex = bufLoc; 
        break; 
      } 
    }
  }

  if (matchLen > 0) {
    // Erase the previously shown keyword
    for (int n=0; n<LastKeywordLenDif; n++) ProcessKey(8);

    // Scan the table for keywords that start with the match buffer
    int entries = tablesize(0) + tablesize(1);
    while (true) {
      bool n = i<tablesize(0);
      const char *k = table(n?0:1)[n?i:i-tablesize(0)].string;
      i = (i + 1) % entries; // Wrap
      if (*k == KybdBuf[bufIndex]) {
        if (strncmp(k, &KybdBuf[bufIndex], matchLen) == 0) {
          // Skip the letters we're matching because they're already there
          LastKeywordLenDif = strlen(k) - matchLen;
          while (*(k + matchLen)) ProcessKey(*(k++ + matchLen));
          return;
        }
      }
      gap++; 
      if (gap == entries) return; // No keywords with this letter
    }
  }
}

// Parenthesis highlighting
void Highlight (int p, uint8_t invert) {
  if (p) {
    for (int n=0; n < p; n++) Display(8);
    Display(17 + invert);
    for (int n=1; n < p; n++) Display(9);
    Display(19 + invert);
    Display(9);
  }
}

void ProcessKey (char c) {
  static int parenthesis = 0;
  static bool string = false;
  if (c == KEY_ESC) { setflag(ESCAPE); return; }    // Escape key
  // Undo previous parenthesis highlight
  Highlight(parenthesis, 0);
  parenthesis = 0;
  // Edit buffer
  if (c == '\n' || c == '\r') {
    pserial('\n');
    KybdAvailable = 1;
    ReadPtr = 0; LastWritePtr = WritePtr;
    return;
  }
  if (c == 8 || c == 0x7f) {     // Backspace key
    if (WritePtr > 0) {
      WritePtr--;
      Display(0x7F);
      if (WritePtr) c = KybdBuf[WritePtr-1];
    }
  } else if (c == SHIFTRETURN) {
    for (int i = 0; i < LastWritePtr; i++) Display(KybdBuf[i]);
    WritePtr = LastWritePtr;
  } else if (WritePtr < KybdBufSize) {
    if (c == '"') string = !string;
    KybdBuf[WritePtr++] = c;
    Display(c);
  }
  // Do new parenthesis highlight
  if (c == ')' && !string) {
    int search = WritePtr-1, level = 0; bool string2 = false;
    while (search >= 0 && parenthesis == 0) {
      c = KybdBuf[search--];
      if (c == '"') string2 = !string2;
      if (c == ')' && !string2) level++;
      if (c == '(' && !string2) {
        level--;
        if (level == 0) parenthesis = WritePtr-search-1;
      }
    }
    Highlight(parenthesis, 1);
  }
  return;
}

