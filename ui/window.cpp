// window.cpp — lightweight PicoCalc text windows for uLisp UI streams.

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "window.h"

extern TFT_eSPI tft;

WindowManager windowManager;

int TextWindow::clampCols(int cols) const {
  if (cols < 1) return 1;
  if (cols > MaxWindowCols) return MaxWindowCols;
  return cols;
}

int TextWindow::clampRows(int rows) const {
  if (rows < 1) return 1;
  if (rows > MaxWindowRows) return MaxWindowRows;
  return rows;
}

void TextWindow::clearCells() {
  for (int row = 0; row < MaxWindowRows; row++) {
    for (int col = 0; col < MaxWindowCols; col++) {
      cells_[row][col] = ' ';
    }
  }
  cursorCol_ = 0;
  cursorRow_ = 0;
}

void TextWindow::clearScreenRect(uint16_t colour) {
#if defined(gfxsupport)
  if (visible_) {
    int width = cols_ * WindowCharWidth + 2 * WindowBorderWidth;
    int height = rows_ * WindowLeading + 2 * WindowBorderWidth;
    tft.fillRect(x_, y_, width, height, colour);
  }
#endif
}

void TextWindow::init(uint8_t id, int x, int y, int cols, int rows) {
  id_ = id;
  used_ = true;
  visible_ = true;
  x_ = x;
  y_ = y;
  cols_ = clampCols(cols);
  rows_ = clampRows(rows);
  fg_ = DefaultWindowForeground;
  bg_ = DefaultWindowBackground;
  border_ = DefaultWindowBorder;
  clearCells();
  dirty_ = true;
}

void TextWindow::clear() {
  clearCells();
  dirty_ = true;
}

void TextWindow::moveTo(int x, int y) {
  clearScreenRect(0x0000);
  x_ = x;
  y_ = y;
  dirty_ = true;
}

void TextWindow::resize(int cols, int rows) {
  clearScreenRect(0x0000);
  cols_ = clampCols(cols);
  rows_ = clampRows(rows);
  clearCells();
  dirty_ = true;
}

void TextWindow::setColors(uint16_t fg, uint16_t bg, uint16_t border) {
  fg_ = fg;
  bg_ = bg;
  border_ = border;
  dirty_ = true;
}

void TextWindow::newline() {
  cursorCol_ = 0;
  cursorRow_++;
  if (cursorRow_ >= rows_) {
    scrollUp();
    cursorRow_ = rows_ - 1;
  }
  dirty_ = true;
}

void TextWindow::scrollUp() {
  for (int row = 1; row < rows_; row++) {
    for (int col = 0; col < cols_; col++) {
      cells_[row - 1][col] = cells_[row][col];
    }
  }
  for (int col = 0; col < cols_; col++) {
    cells_[rows_ - 1][col] = ' ';
  }
}

void TextWindow::writeChar(char c) {
  if (!used_ || !visible_) return;
  if (c == '\r') return;
  if (c == '\n') { newline(); return; }
  if ((static_cast<uint8_t>(c) & 0x7F) < 32) return;

  cells_[cursorRow_][cursorCol_] = c;
  cursorCol_++;
  if (cursorCol_ >= cols_) newline();
  dirty_ = true;
}

void TextWindow::writeString(const char *s) {
  while (*s) writeChar(*s++);
}

void TextWindow::renderIfDirty() {
#if defined(gfxsupport)
  if (!used_ || !visible_ || !dirty_) return;
  int width = cols_ * WindowCharWidth + 2 * WindowBorderWidth;
  int height = rows_ * WindowLeading + 2 * WindowBorderWidth;
  tft.fillRect(x_, y_, width, height, bg_);
  tft.drawRect(x_, y_, width, height, border_);
  for (int row = 0; row < rows_; row++) {
    for (int col = 0; col < cols_; col++) {
      int px = x_ + WindowBorderWidth + col * WindowCharWidth;
      int py = y_ + WindowBorderWidth + row * WindowLeading;
      char ch = cells_[row][col];
      if (ch != ' ') tft.drawChar(px, py, ch, fg_, bg_, 1);
    }
  }
  dirty_ = false;
#endif
}

void TextWindow::close() {
  clearScreenRect(0x0000);
  used_ = false;
  visible_ = false;
  dirty_ = false;
}

void WindowManager::init() {
  focused_ = 0xFF;
  for (uint8_t i = 0; i < MaxTextWindows; i++) {
    windows_[i].close();
  }
}

TextWindow *WindowManager::createTextWindow(int x, int y, int cols, int rows) {
  for (uint8_t i = 0; i < MaxTextWindows; i++) {
    if (!windows_[i].used()) {
      windows_[i].init(i, x, y, cols, rows);
      focused_ = i;
      return &windows_[i];
    }
  }
  return nullptr;
}

TextWindow *WindowManager::textWindow(uint8_t id) {
  if (id >= MaxTextWindows || !windows_[id].used()) return nullptr;
  return &windows_[id];
}

bool WindowManager::close(uint8_t id) {
  TextWindow *window = textWindow(id);
  if (window == nullptr) return false;
  window->close();
  if (focused_ == id) focused_ = 0xFF;
  return true;
}

bool WindowManager::move(uint8_t id, int x, int y) {
  TextWindow *window = textWindow(id);
  if (window == nullptr) return false;
  window->moveTo(x, y);
  return true;
}

bool WindowManager::resize(uint8_t id, int cols, int rows) {
  TextWindow *window = textWindow(id);
  if (window == nullptr) return false;
  window->resize(cols, rows);
  return true;
}

bool WindowManager::focus(uint8_t id) {
  if (textWindow(id) == nullptr) return false;
  focused_ = id;
  return true;
}

void WindowManager::renderAllDirty() {
  for (uint8_t i = 0; i < MaxTextWindows; i++) {
    windows_[i].renderIfDirty();
  }
}
