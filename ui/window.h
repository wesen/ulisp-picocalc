// window.h — lightweight PicoCalc text windows for uLisp UI streams.

#ifndef ULISP_UI_WINDOW_H
#define ULISP_UI_WINDOW_H

#include <Arduino.h>

constexpr uint8_t MaxTextWindows = 4;
constexpr uint8_t MaxWindowCols = 53;
constexpr uint8_t MaxWindowRows = 16;
constexpr uint8_t WindowCharWidth = 6;
constexpr uint8_t WindowLeading = 10;
constexpr uint8_t WindowBorderWidth = 1;
constexpr uint16_t DefaultWindowForeground = 0xFFFF;
constexpr uint16_t DefaultWindowBackground = 0x0010;
constexpr uint16_t DefaultWindowBorder = 0x07FF;

class TextWindow {
public:
  void init(uint8_t id, int x, int y, int cols, int rows);
  void clear();
  void moveTo(int x, int y);
  void resize(int cols, int rows);
  void setColors(uint16_t fg, uint16_t bg, uint16_t border);
  void writeChar(char c);
  void writeString(const char *s);
  void renderIfDirty();
  void close();

  uint8_t id() const { return id_; }
  bool used() const { return used_; }
  bool visible() const { return visible_; }
  int x() const { return x_; }
  int y() const { return y_; }
  int cols() const { return cols_; }
  int rows() const { return rows_; }

private:
  void newline();
  void scrollUp();
  void clearCells();
  void clearScreenRect(uint16_t colour);
  int clampCols(int cols) const;
  int clampRows(int rows) const;

  uint8_t id_ = 0;
  bool used_ = false;
  bool visible_ = false;
  bool dirty_ = false;
  int x_ = 0;
  int y_ = 0;
  int cols_ = MaxWindowCols;
  int rows_ = MaxWindowRows;
  int cursorCol_ = 0;
  int cursorRow_ = 0;
  uint16_t fg_ = DefaultWindowForeground;
  uint16_t bg_ = DefaultWindowBackground;
  uint16_t border_ = DefaultWindowBorder;
  char cells_[MaxWindowRows][MaxWindowCols];
};

class WindowManager {
public:
  void init();
  TextWindow *createTextWindow(int x, int y, int cols, int rows);
  TextWindow *textWindow(uint8_t id);
  bool close(uint8_t id);
  bool move(uint8_t id, int x, int y);
  bool resize(uint8_t id, int cols, int rows);
  bool focus(uint8_t id);
  uint8_t focused() const { return focused_; }
  void renderAllDirty();

private:
  TextWindow windows_[MaxTextWindows];
  uint8_t focused_ = 0xFF;
};

extern WindowManager windowManager;

#endif // ULISP_UI_WINDOW_H
