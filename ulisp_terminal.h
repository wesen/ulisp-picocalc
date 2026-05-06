// ulisp_terminal.h — legacy PicoCalc terminal and keyboard helpers.

#ifndef ULISP_TERMINAL_H
#define ULISP_TERMINAL_H

#include <Arduino.h>

extern const int Columns;
extern const int Lines;
extern volatile int WritePtr;
extern volatile int ReadPtr;
extern volatile int LastWritePtr;
extern const int KybdBufSize;
extern char KybdBuf[];
extern volatile uint8_t KybdAvailable;

void PlotChar(uint8_t ch, uint8_t line, uint8_t column);
void ScrollDisplay();
void Display(char c);
void initkybd();
void autoComplete();
void Highlight(int p, uint8_t invert);
void ProcessKey(char c);

#endif // ULISP_TERMINAL_H
