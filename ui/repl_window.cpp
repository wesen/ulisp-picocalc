// repl_window.cpp — REPL window state and implementation.

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>
#include "repl_window.h"
#include "window.h"

ReplWindow mainReplWindow;

ReplDrawState &replDrawState = mainReplWindow.drawState;
ReplBackBuffer &replBack = mainReplWindow.back;
ReplEditBuffer &replEdit = mainReplWindow.edit;
ReplRenderCell (&replDesired)[ReplLines][ReplColumns] = mainReplWindow.desired;
ReplRenderCell (&replDrawn)[ReplLines][ReplColumns] = mainReplWindow.drawn;
bool &replDrawnValid = mainReplWindow.drawnValid;
bool &replUiDirty = mainReplWindow.uiDirty;

// ---------------------------------------------------------------------------
// Back buffer helpers
// ---------------------------------------------------------------------------

uint16_t replLogicalFirstRow() {
  if (replBack.count < ReplBackBufferRows) return 0;
  return (replBack.currentRow + 1) % ReplBackBufferRows;
}

uint16_t replPhysicalForLogical(uint16_t logical) {
  return (replLogicalFirstRow() + logical) % ReplBackBufferRows;
}

void replClearBackRow(uint16_t row) {
  for (int c = 0; c < ReplColumns; c++) {
    replBack.rows[row][c] = ' ';
    replBack.attrs[row][c] = ReplPackAttr(1, 0);
    replBack.bold[row][c] = false;
  }
}

void replScrollToTail() {
  if (replBack.count <= ReplTranscriptRows) replBack.viewportStart = 0;
  else replBack.viewportStart = replBack.count - ReplTranscriptRows;
}

void replAdvanceRow() {
  replBack.currentRow = (replBack.currentRow + 1) % ReplBackBufferRows;
  replClearBackRow(replBack.currentRow);
  replBack.currentCol = 0;
  if (replBack.count < ReplBackBufferRows) replBack.count++;
  if (replBack.followTail) replScrollToTail();
}

void ReplBackBufferAppend(char c) {
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

void ReplBackBufferWriteString(const char *s) {
  while (*s) ReplBackBufferAppend(*s++);
}

void ReplResetDrawState() {
  replDrawState.fg = 1;
  replDrawState.bg = 0;
  replDrawState.bold = false;
}

void ReplSetOutputStyle() {
  replDrawState.fg = 1;   // soft white
  replDrawState.bg = 0;   // black
  replDrawState.bold = false;
}

void ReplSetPromptStyle() {
  replDrawState.fg = 4;   // neon cyan
  replDrawState.bg = 0;
  replDrawState.bold = true;
}

void ReplSetInputStyle() {
  replDrawState.fg = 3;   // neon green
  replDrawState.bg = 0;
  replDrawState.bold = false;
}

void ReplSetErrorStyle() {
  replDrawState.fg = 2;   // neon pink
  replDrawState.bg = 0;
  replDrawState.bold = true;
}

void ReplWindowInit() {
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

void ReplEditReset() {
  replEdit.len = 0;
  replEdit.cursor = 0;
  replEdit.text[0] = '\0';
  replEdit.committed = false;
  replEdit.readPos = 0;
}

bool ReplEditInsert(char c) {
  if (replEdit.len >= ReplInputBufferSize - 1) return false;
  for (int i = replEdit.len; i > replEdit.cursor; i--) replEdit.text[i] = replEdit.text[i - 1];
  replEdit.text[replEdit.cursor++] = c;
  replEdit.text[++replEdit.len] = '\0';
  return true;
}

bool ReplEditBackspace() {
  if (replEdit.cursor == 0) return false;
  for (int i = replEdit.cursor - 1; i < replEdit.len - 1; i++) replEdit.text[i] = replEdit.text[i + 1];
  replEdit.cursor--;
  replEdit.text[--replEdit.len] = '\0';
  return true;
}

bool ReplEditDelete() {
  if (replEdit.cursor >= replEdit.len) return false;
  for (int i = replEdit.cursor; i < replEdit.len - 1; i++) replEdit.text[i] = replEdit.text[i + 1];
  replEdit.text[--replEdit.len] = '\0';
  return true;
}

void ReplEditMoveLeft() { if (replEdit.cursor > 0) replEdit.cursor--; }
void ReplEditMoveRight() { if (replEdit.cursor < replEdit.len) replEdit.cursor++; }

void ReplEditCommit() {
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

uint16_t replCellFg(int row, int col) {
  return ReplPalette[ReplAttrFg(replDesired[row][col].attr)];
}

uint16_t replCellBg(int row, int col) {
  return ReplPalette[ReplAttrBg(replDesired[row][col].attr)];
}

bool replCellsEqual(int row, int col) {
  return replDesired[row][col].ch == replDrawn[row][col].ch &&
         replDesired[row][col].attr == replDrawn[row][col].attr &&
         replDesired[row][col].bold == replDrawn[row][col].bold;
}

void replSetDesired(int row, int col, char ch, uint8_t attr, bool bold) {
  if (row < 0 || row >= ReplLines || col < 0 || col >= ReplColumns) return;
  replDesired[row][col].ch = ch;
  replDesired[row][col].attr = attr;
  replDesired[row][col].bold = bold;
}

void replClearDesired() {
  for (int r = 0; r < ReplLines; r++)
    for (int c = 0; c < ReplColumns; c++)
      replSetDesired(r, c, ' ', ReplPackAttr(1, 0), false);
}

void replSetDesiredText(int row, int col, const char *text, uint8_t attr, bool bold) {
  while (*text && col < ReplColumns) replSetDesired(row, col++, *text++, attr, bold);
}

void replDrawCell(int row, int col) {
  int x = ReplWindowX + col * ReplCharWidth;
  int y = ReplWindowY + row * ReplLeading;
  uint16_t fg = replCellFg(row, col);
  uint16_t bg = replCellBg(row, col);
  char ch = replDesired[row][col].ch;
  tft.fillRect(x, y, ReplCharWidth, ReplLeading, bg);
  if (ch != ' ') {
    tft.drawChar(x, y, ch, fg, bg, 1);
    if (replDesired[row][col].bold && x + 1 < ReplWindowX + ReplWindowWidth)
      tft.drawChar(x + 1, y, ch, fg, bg, 1);
  }
}

void replComposeTranscript() {
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

void replComposeStatus() {
  int row = ReplTranscriptRows;
  uint8_t attr = ReplPackAttr(0, 11);
  for (int c = 0; c < ReplColumns; c++) replSetDesired(row, c, ' ', attr, false);
  char status[80];
  snprintf(status, sizeof(status), "rows %u/%u view %u cursor %u len %u",
           replBack.count, ReplBackBufferRows, replBack.viewportStart,
           replEdit.cursor, replEdit.len);
  replSetDesiredText(row, 0, status, attr, false);
}

void replComposeInput() {
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

void ReplRenderAll() {
  tft.drawFastHLine(ReplWindowX, ReplWindowY, ReplWindowWidth, ReplPalette[4]);
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
  windowManager.renderAll();
}

void ReplRenderIfDirty() {
  if (replUiDirty) ReplRenderAll();
}

// ---------------------------------------------------------------------------
// Keyboard processing
// ---------------------------------------------------------------------------

void ReplWindow::init() { ReplWindowInit(); }
void ReplWindow::appendOutput(char c) { ReplBackBufferAppend(c); }
void ReplWindow::writeOutputString(const char *s) { ReplBackBufferWriteString(s); }
void ReplWindow::resetDrawState() { ReplResetDrawState(); }
void ReplWindow::setOutputStyle() { ReplSetOutputStyle(); }
void ReplWindow::setPromptStyle() { ReplSetPromptStyle(); }
void ReplWindow::setInputStyle() { ReplSetInputStyle(); }
void ReplWindow::setErrorStyle() { ReplSetErrorStyle(); }
void ReplWindow::processKey(uint8_t key) { ReplProcessKey(key); }
void ReplWindow::renderAll() { ReplRenderAll(); }
void ReplWindow::renderIfDirty() { ReplRenderIfDirty(); }

void ReplProcessKey(uint8_t key) {
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

