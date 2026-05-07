// repl_window.h — REPL window for uLisp on PicoCalc
//
// Extracted from repl-window-primitives.ino for integration into
// ulisp-picocalc.ino.  Provides:
//   - back-buffer transcript with per-cell fg/bg/bold
//   - editable input line with cursor
//   - dirty character-cell renderer
//   - 16-color neon/neotokyo palette
//
// This header depends on TFT_eSPI (tft) and PCKeyboard (pc_kbd)
// being declared in the including .ino file.

#ifndef REPL_WINDOW_H
#define REPL_WINDOW_H

#include <Arduino.h>
#include <TFT_eSPI.h>
// Do not include PCKeyboard.h here: the library header has no include guard.
// ulisp-picocalc.ino includes PCKeyboard.h before this header, so the type is
// already available for the extern declaration below.

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

constexpr int ReplScreenWidth = 320;
constexpr int ReplScreenHeight = 320;
constexpr int ReplWindowX = 0;
constexpr int ReplWindowY = ReplScreenHeight / 2;
constexpr int ReplWindowWidth = ReplScreenWidth;
constexpr int ReplWindowHeight = ReplScreenHeight / 2;
constexpr int ReplCharWidth = 6;
constexpr int ReplCharHeight = 8;
constexpr int ReplLeading = 10;
constexpr int ReplColumns = ReplWindowWidth / ReplCharWidth;   // 53
constexpr int ReplLines = ReplWindowHeight / ReplLeading;      // 16
constexpr int ReplStatusRows = 1;
constexpr int ReplInputRows = 3;
constexpr int ReplTranscriptRows = ReplLines - ReplStatusRows - ReplInputRows;
constexpr int ReplBackBufferRows = 160;
constexpr int ReplInputBufferSize = 512;

// ---------------------------------------------------------------------------
// Palette (RGB565 neon / neotokyo)
// ---------------------------------------------------------------------------

const uint16_t ReplPalette[16] = {
  0x0000,  //  0 void black
  0xCE79,  //  1 soft white
  0xF81F,  //  2 neon pink
  0x07E0,  //  3 neon green
  0x07FF,  //  4 neon cyan
  0xFFE0,  //  5 neon yellow
  0xD81F,  //  6 neon purple
  0xFD20,  //  7 neon orange
  0x001F,  //  8 electric blue
  0xF8B9,  //  9 hot magenta
  0xA7E4,  // 10 acid lime
  0x87FF,  // 11 ice blue
  0xFC80,  // 12 coral
  0x05BF,  // 13 deep cyan
  0xBA9A,  // 14 muted pink
  0x10A2,  // 15 dark navy
};

inline uint8_t ReplPackAttr(uint8_t fg, uint8_t bg) { return ((fg & 0x0F) << 4) | (bg & 0x0F); }
inline uint8_t ReplAttrFg(uint8_t attr) { return (attr >> 4) & 0x0F; }
inline uint8_t ReplAttrBg(uint8_t attr) { return attr & 0x0F; }

// ---------------------------------------------------------------------------
// State structs
// ---------------------------------------------------------------------------

struct ReplDrawState {
  uint8_t fg = 1;
  uint8_t bg = 0;
  bool bold = false;
};

struct ReplBackBuffer {
  char rows[ReplBackBufferRows][ReplColumns];
  uint8_t attrs[ReplBackBufferRows][ReplColumns];
  bool bold[ReplBackBufferRows][ReplColumns];
  uint16_t currentRow = 0;
  uint16_t currentCol = 0;
  uint16_t count = 1;
  uint16_t viewportStart = 0;
  bool followTail = true;
};

struct ReplEditBuffer {
  char text[ReplInputBufferSize];
  uint16_t len = 0;
  uint16_t cursor = 0;
  bool committed = false;
  uint16_t readPos = 0;
};

struct ReplRenderCell {
  char ch;
  uint8_t attr;
  bool bold;
};

// ---------------------------------------------------------------------------
// Globals (defined in the header for single-file inclusion simplicity)
// ---------------------------------------------------------------------------

extern TFT_eSPI tft;
extern PCKeyboard pc_kbd;

extern ReplDrawState replDrawState;
extern ReplBackBuffer replBack;
extern ReplEditBuffer replEdit;
extern ReplRenderCell replDesired[ReplLines][ReplColumns];
extern ReplRenderCell replDrawn[ReplLines][ReplColumns];
extern bool replDrawnValid;
extern bool replUiDirty;

// ---------------------------------------------------------------------------
// Back buffer / draw-state API
// ---------------------------------------------------------------------------

uint16_t replLogicalFirstRow();
uint16_t replPhysicalForLogical(uint16_t logical);
void replClearBackRow(uint16_t row);
void replScrollToTail();
void replAdvanceRow();

void ReplBackBufferAppend(char c);
void ReplBackBufferWriteString(const char *s);
void ReplResetDrawState();
void ReplSetOutputStyle();
void ReplSetPromptStyle();
void ReplSetInputStyle();
void ReplSetErrorStyle();
void ReplWindowInit();

// ---------------------------------------------------------------------------
// Edit buffer API
// ---------------------------------------------------------------------------

void ReplEditReset();
bool ReplEditInsert(char c);
bool ReplEditBackspace();
bool ReplEditDelete();
void ReplEditMoveLeft();
void ReplEditMoveRight();
void ReplEditCommit();

// ---------------------------------------------------------------------------
// Renderer API
// ---------------------------------------------------------------------------

uint16_t replCellFg(int row, int col);
uint16_t replCellBg(int row, int col);
bool replCellsEqual(int row, int col);
void replSetDesired(int row, int col, char ch, uint8_t attr, bool bold);
void replClearDesired();
void replSetDesiredText(int row, int col, const char *text, uint8_t attr, bool bold);
void replDrawCell(int row, int col);
void replComposeTranscript();
void replComposeStatus();
void replComposeInput();
void ReplRenderAll();
void ReplRenderIfDirty();

// ---------------------------------------------------------------------------
// Keyboard processing
// ---------------------------------------------------------------------------

void ReplProcessKey(uint8_t key);

#endif // REPL_WINDOW_H
