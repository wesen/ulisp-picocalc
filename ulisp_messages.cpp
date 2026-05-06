// ulisp_messages.cpp — shared text and PicoCalc constants with external linkage.

#include <Arduino.h>
#include "ulisp_messages.h"

extern const char LispLibrary[] = "";

extern const int COLOR_WHITE = 0xffff;
extern const int COLOR_BLACK = 0;
extern const int KEY_ESC = 0xB1;

// Save space as these are used multiple times
extern const char notanumber[] = "argument is not a number";
extern const char notaninteger[] = "argument is not an integer";
extern const char notastring[] = "argument is not a string";
extern const char notalist[] = "argument is not a list";
extern const char notasymbol[] = "argument is not a symbol";
extern const char notproper[] = "argument is not a proper list";
extern const char toomanyargs[] = "too many arguments";
extern const char toofewargs[] = "too few arguments";
extern const char noargument[] = "missing argument";
extern const char nostream[] = "missing stream argument";
extern const char overflow[] = "arithmetic overflow";
extern const char divisionbyzero[] = "division by zero";
extern const char indexnegative[] = "index can't be negative";
extern const char invalidarg[] = "invalid argument";
extern const char invalidkey[] = "invalid keyword";
extern const char illegalclause[] = "illegal clause";
extern const char illegalfn[] = "illegal function";
extern const char invalidpin[] = "invalid pin";
extern const char oddargs[] = "odd number of arguments";
extern const char indexrange[] = "index out of range";
extern const char canttakecar[] = "can't take car";
extern const char canttakecdr[] = "can't take cdr";
extern const char unknownstreamtype[] = "unknown stream type";
