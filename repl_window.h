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
constexpr int ReplCharWidth = 6;
constexpr int ReplCharHeight = 8;
constexpr int ReplLeading = 10;
constexpr int ReplColumns = ReplScreenWidth / ReplCharWidth;   // 53
constexpr int ReplLines = ReplScreenHeight / ReplLeading;      // 32
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
// Back buffer helpers
// ---------------------------------------------------------------------------

inline uint16_t replLogicalFirstRow() {
  if (replBack.count < ReplBackBufferRows) return 0;
  return (replBack.currentRow + 1) % ReplBackBufferRows;
}

inline uint16_t replPhysicalForLogical(uint16_t logical) {
  return (replLogicalFirstRow() + logical) % ReplBackBufferRows;
}

inline void replClearBackRow(uint16_t row) {
  for (int c = 0; c < ReplColumns; c++) {
    replBack.rows[row][c] = ' ';
    replBack.attrs[row][c] = ReplPackAttr(1, 0);
    replBack.bold[row][c] = false;
  }
}

inline void replScrollToTail() {
  if (replBack.count <= ReplTranscriptRows) replBack.viewportStart = 0;
  else replBack.viewportStart = replBack.count - ReplTranscriptRows;
}

inline void replAdvanceRow() {
  replBack.currentRow = (replBack.currentRow + 1) % ReplBackBufferRows;
  replClearBackRow(replBack.currentRow);
  replBack.currentCol = 0;
  if (replBack.count < ReplBackBufferRows) replBack.count++;
  if (replBack.followTail) replScrollToTail();
}

inline void ReplBackBufferAppend(char c) {
  if (c == '\r') return;
  if (c == '\n') { replAdvanceRow(); replUiDirty = true; return; }
  if ((static_cast<uint8_t>(c) & 0x7F) < 32) return;
  replBack.rows[replBack.currentRow][replBack.currentCol] = c;
  replBack.attrs[replBack.currentRow][replBack.currentCol] = ReplPackAttr(replDrawState.fg, replDrawState.bg);
  replBack.bold[replBack.currentRow][replBack.currentCol] = replDrawState.bold;
  replBack.currentCol++;
  if (replBack.currentCol >= ReplColumns) replAdvanceRow();
  if (replBack.followTail) replScrollToTail();
  replUiDirty = true;
}

inline void ReplBackBufferWriteString(const char *s) {
  while (*s) ReplBackBufferAppend(*s++);
}

inline void ReplResetDrawState() {
  replDrawState.fg = 1;
  replDrawState.bg = 0;
  replDrawState.bold = false;
}

inline void ReplSetOutputStyle() {
  replDrawState.fg = 1;   // soft white
  replDrawState.bg = 0;   // black
  replDrawState.bold = false;
}

inline void ReplSetPromptStyle() {
  replDrawState.fg = 4;   // neon cyan
  replDrawState.bg = 0;
  replDrawState.bold = true;
}

inline void ReplSetInputStyle() {
  replDrawState.fg = 3;   // neon green
  replDrawState.bg = 0;
  replDrawState.bold = false;
}

inline void ReplSetErrorStyle() {
  replDrawState.fg = 2;   // neon pink
  replDrawState.bg = 0;
  replDrawState.bold = true;
}

inline void ReplWindowInit() {
  for (int r = 0; r < ReplBackBufferRows; r++) replClearBackRow(r);
  replBack.currentRow = 0;
  replBack.currentCol = 0;
  replBack.count = 1;
  replBack.viewportStart = 0;
  replBack.followTail = true;
  ReplResetDrawState();
  replEdit.len = 0;
  replEdit.cursor = 0;
  replEdit.committed = false;
  replEdit.readPos = 0;
  replDrawnValid = false;
  replUiDirty = true;
}

// ---------------------------------------------------------------------------
// Edit buffer helpers
// ---------------------------------------------------------------------------

inline void ReplEditReset() {
  replEdit.len = 0;
  replEdit.cursor = 0;
  replEdit.text[0] = '\0';
  replEdit.committed = false;
  replEdit.readPos = 0;
}

inline bool ReplEditInsert(char c) {
  if (replEdit.len >= ReplInputBufferSize - 1) return false;
  for (int i = replEdit.len; i > replEdit.cursor; i--) replEdit.text[i] = replEdit.text[i - 1];
  replEdit.text[replEdit.cursor++] = c;
  replEdit.text[++replEdit.len] = '\0';
  return true;
}

inline bool ReplEditBackspace() {
  if (replEdit.cursor == 0) return false;
  for (int i = replEdit.cursor - 1; i < replEdit.len - 1; i++) replEdit.text[i] = replEdit.text[i + 1];
  replEdit.cursor--;
  replEdit.text[--replEdit.len] = '\0';
  return true;
}

inline bool ReplEditDelete() {
  if (replEdit.cursor >= replEdit.len) return false;
  for (int i = replEdit.cursor; i < replEdit.len - 1; i++) replEdit.text[i] = replEdit.text[i + 1];
  replEdit.text[--replEdit.len] = '\0';
  return true;
}

inline void ReplEditMoveLeft() { if (replEdit.cursor > 0) replEdit.cursor--; }
inline void ReplEditMoveRight() { if (replEdit.cursor < replEdit.len) replEdit.cursor++; }

inline void ReplEditCommit() {
  // The REPL loop already printed the prompt into the transcript.  On commit,
  // append only the edited input text, in input color, then terminate the line.
  ReplSetInputStyle();
  for (uint16_t i = 0; i < replEdit.len; i++) ReplBackBufferAppend(replEdit.text[i]);
  ReplBackBufferAppend('\n');
  ReplSetOutputStyle();
  // Mark committed for gserial consumption
  replEdit.committed = true;
  replEdit.readPos = 0;
}

// ---------------------------------------------------------------------------
// Renderer
// ---------------------------------------------------------------------------

inline uint16_t replCellFg(int row, int col) {
  return ReplPalette[ReplAttrFg(replDesired[row][col].attr)];
}

inline uint16_t replCellBg(int row, int col) {
  return ReplPalette[ReplAttrBg(replDesired[row][col].attr)];
}

inline bool replCellsEqual(int row, int col) {
  return replDesired[row][col].ch == replDrawn[row][col].ch &&
         replDesired[row][col].attr == replDrawn[row][col].attr &&
         replDesired[row][col].bold == replDrawn[row][col].bold;
}

inline void replSetDesired(int row, int col, char ch, uint8_t attr, bool bold) {
  if (row < 0 || row >= ReplLines || col < 0 || col >= ReplColumns) return;
  replDesired[row][col].ch = ch;
  replDesired[row][col].attr = attr;
  replDesired[row][col].bold = bold;
}

inline void replClearDesired() {
  for (int r = 0; r < ReplLines; r++)
    for (int c = 0; c < ReplColumns; c++)
      replSetDesired(r, c, ' ', ReplPackAttr(1, 0), false);
}

inline void replSetDesiredText(int row, int col, const char *text, uint8_t attr, bool bold) {
  while (*text && col < ReplColumns) replSetDesired(row, col++, *text++, attr, bold);
}

inline void replDrawCell(int row, int col) {
  int x = col * ReplCharWidth;
  int y = row * ReplLeading;
  uint16_t fg = replCellFg(row, col);
  uint16_t bg = replCellBg(row, col);
  char ch = replDesired[row][col].ch;
  tft.fillRect(x, y, ReplCharWidth, ReplLeading, bg);
  if (ch != ' ') {
    tft.drawChar(x, y, ch, fg, bg, 1);
    if (replDesired[row][col].bold && x + 1 < ReplScreenWidth)
      tft.drawChar(x + 1, y, ch, fg, bg, 1);
  }
}

inline void replComposeTranscript() {
  for (int row = 0; row < ReplTranscriptRows; row++) {
    uint16_t logical = replBack.viewportStart + row;
    if (logical >= replBack.count) continue;
    uint16_t physical = replPhysicalForLogical(logical);
    for (int col = 0; col < ReplColumns; col++) {
      char ch = replBack.rows[physical][col];
      if (ch != ' ') replSetDesired(row, col, ch, replBack.attrs[physical][col], replBack.bold[physical][col]);
    }
  }
}

inline void replComposeStatus() {
  int row = ReplTranscriptRows;
  uint8_t attr = ReplPackAttr(0, 11);
  for (int c = 0; c < ReplColumns; c++) replSetDesired(row, c, ' ', attr, false);
  char status[80];
  snprintf(status, sizeof(status), "rows %u/%u view %u cursor %u len %u",
           replBack.count, ReplBackBufferRows, replBack.viewportStart,
           replEdit.cursor, replEdit.len);
  replSetDesiredText(row, 0, status, attr, false);
}

inline void replComposeInput() {
  int firstRow = ReplTranscriptRows + ReplStatusRows;
  replSetDesiredText(firstRow, 0, "> ", ReplPackAttr(3, 0), false);
  int screenRow = firstRow;
  int screenCol = 2;
  for (uint16_t i = 0; i < replEdit.len && screenRow < ReplLines; i++) {
    if (screenCol >= ReplColumns) {
      screenCol = 0;
      screenRow++;
      if (screenRow >= ReplLines) break;
    }
    replSetDesired(screenRow, screenCol++, replEdit.text[i], ReplPackAttr(1, 0), false);
  }
  uint16_t cursorVisual = replEdit.cursor + 2;
  int cursorRow = firstRow + (cursorVisual / ReplColumns);
  int cursorCol = cursorVisual % ReplColumns;
  if (cursorRow < ReplLines) {
    char under = ' ';
    if (replEdit.cursor < replEdit.len) under = replEdit.text[replEdit.cursor];
    replSetDesired(cursorRow, cursorCol, under == ' ' ? '_' : under, ReplPackAttr(0, 3), false);
  }
}

inline void ReplRenderAll() {
  replClearDesired();
  replComposeTranscript();
  replComposeStatus();
  replComposeInput();
  for (int r = 0; r < ReplLines; r++) {
    for (int c = 0; c < ReplColumns; c++) {
      if (!replDrawnValid || !replCellsEqual(r, c)) {
        replDrawCell(r, c);
        replDrawn[r][c] = replDesired[r][c];
      }
    }
  }
  replDrawnValid = true;
  replUiDirty = false;
}

inline void ReplRenderIfDirty() {
  if (replUiDirty) ReplRenderAll();
}

// ---------------------------------------------------------------------------
// Keyboard processing
// ---------------------------------------------------------------------------

inline void ReplProcessKey(uint8_t key) {
  if (key == 0xB1) { // KEY_ESC
    // Handled by caller (setflag ESCAPE)
    return;
  }
  if (key == '\r' || key == '\n') {
    ReplEditCommit();
    return;
  }
  if (key == 8 || key == 0x7F) {
    ReplEditBackspace();
    replUiDirty = true;
    return;
  }
  if (key == 0xA1) { // candidate LEFT
    ReplEditMoveLeft();
    replUiDirty = true;
    return;
  }
  if (key == 0xA2) { // candidate RIGHT
    ReplEditMoveRight();
    replUiDirty = true;
    return;
  }
  if (key == 0xA3) { // candidate UP
    if (replBack.viewportStart > 0) { replBack.viewportStart--; replBack.followTail = false; }
    replUiDirty = true;
    return;
  }
  if (key == 0xA4) { // candidate DOWN
    if (replBack.viewportStart + ReplTranscriptRows < replBack.count) replBack.viewportStart++;
    else { replBack.followTail = true; replScrollToTail(); }
    replUiDirty = true;
    return;
  }
  if (key == 0xA5) { // candidate DELETE
    ReplEditDelete();
    replUiDirty = true;
    return;
  }
  if (key >= 32 && key < 127) {
    ReplEditInsert(static_cast<char>(key));
    replUiDirty = true;
    return;
  }
  // Unknown key: log as back-buffer text for debugging
  ReplBackBufferAppend('[');
  ReplBackBufferAppend('?');
  ReplBackBufferAppend(']');
  replUiDirty = true;
}

#endif // REPL_WINDOW_H
