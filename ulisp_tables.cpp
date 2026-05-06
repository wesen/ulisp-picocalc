#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>

#if defined(sdcardsupport)
#include <SD.h>
#endif

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_messages.h"
#include "ulisp_state.h"
#include "ulisp_tables.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_arduino.h"
#include "ulisp_gfx.h"
#include "ulisp_picocalc.h"
#include "ulisp_runtime.h"
#include "ulisp_streams.h"
#include "ulisp_platform.h"
#include "ulisp_pretty.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_print.h"
#include "ulisp_terminal.h"
#include "ulisp_builtins.h"
#include "ulisp_entry.h"

#define Serial Serial1

extern PCKeyboard pc_kbd;
extern const int KEY_ESC;
extern const char toofewargs[];
extern const char toomanyargs[];

// Built-in symbol names
const char string0[] = "nil";
const char string1[] = "t";
const char string2[] = "nothing";
const char string3[] = "&optional";
const char string4[] = "*features*";
const char string5[] = ":initial-element";
const char string6[] = ":element-type";
const char string7[] = ":test";
const char string8[] = ":a";
const char string9[] = ":b";
const char string10[] = ":c";
const char string11[] = "bit";
const char string12[] = "&rest";
const char string13[] = "lambda";
const char string14[] = "let";
const char string15[] = "let*";
const char string16[] = "closure";
const char string17[] = "*pc*";
const char string18[] = "quote";
const char string19[] = "defun";
const char string20[] = "defvar";
const char string21[] = "defcode";
const char string22[] = "eq";
const char string23[] = "car";
const char string24[] = "first";
const char string25[] = "cdr";
const char string26[] = "rest";
const char string27[] = "nth";
const char string28[] = "aref";
const char string29[] = "char";
const char string30[] = "string";
const char string31[] = "pinmode";
const char string32[] = "digitalwrite";
const char string33[] = "analogread";
const char string34[] = "analogreference";
const char string35[] = "register";
const char string36[] = "format";
const char string37[] = "or";
const char string38[] = "setq";
const char string39[] = "loop";
const char string40[] = "push";
const char string41[] = "pop";
const char string42[] = "incf";
const char string43[] = "decf";
const char string44[] = "setf";
const char string45[] = "dolist";
const char string46[] = "dotimes";
const char string47[] = "do";
const char string48[] = "do*";
const char string49[] = "trace";
const char string50[] = "untrace";
const char string51[] = "for-millis";
const char string52[] = "time";
const char string53[] = "with-output-to-string";
const char string54[] = "with-serial";
const char string55[] = "with-i2c";
const char string56[] = "with-spi";
const char string57[] = "with-sd-card";
const char string58[] = "progn";
const char string59[] = "if";
const char string60[] = "cond";
const char string61[] = "when";
const char string62[] = "unless";
const char string63[] = "case";
const char string64[] = "and";
const char string65[] = "not";
const char string66[] = "null";
const char string67[] = "cons";
const char string68[] = "atom";
const char string69[] = "listp";
const char string70[] = "consp";
const char string71[] = "symbolp";
const char string72[] = "arrayp";
const char string73[] = "boundp";
const char string74[] = "keywordp";
const char string75[] = "set";
const char string76[] = "streamp";
const char string77[] = "equal";
const char string78[] = "caar";
const char string79[] = "cadr";
const char string80[] = "second";
const char string81[] = "cdar";
const char string82[] = "cddr";
const char string83[] = "caaar";
const char string84[] = "caadr";
const char string85[] = "cadar";
const char string86[] = "caddr";
const char string87[] = "third";
const char string88[] = "cdaar";
const char string89[] = "cdadr";
const char string90[] = "cddar";
const char string91[] = "cdddr";
const char string92[] = "length";
const char string93[] = "array-dimensions";
const char string94[] = "list";
const char string95[] = "copy-list";
const char string96[] = "make-array";
const char string97[] = "reverse";
const char string98[] = "assoc";
const char string99[] = "member";
const char string100[] = "apply";
const char string101[] = "funcall";
const char string102[] = "append";
const char string103[] = "mapc";
const char string104[] = "mapl";
const char string105[] = "mapcar";
const char string106[] = "mapcan";
const char string107[] = "maplist";
const char string108[] = "mapcon";
const char string109[] = "+";
const char string110[] = "-";
const char string111[] = "*";
const char string112[] = "/";
const char string113[] = "mod";
const char string114[] = "rem";
const char string115[] = "1+";
const char string116[] = "1-";
const char string117[] = "abs";
const char string118[] = "random";
const char string119[] = "max";
const char string120[] = "min";
const char string121[] = "/=";
const char string122[] = "=";
const char string123[] = "<";
const char string124[] = "<=";
const char string125[] = ">";
const char string126[] = ">=";
const char string127[] = "plusp";
const char string128[] = "minusp";
const char string129[] = "zerop";
const char string130[] = "oddp";
const char string131[] = "evenp";
const char string132[] = "integerp";
const char string133[] = "numberp";
const char string134[] = "float";
const char string135[] = "floatp";
const char string136[] = "sin";
const char string137[] = "cos";
const char string138[] = "tan";
const char string139[] = "asin";
const char string140[] = "acos";
const char string141[] = "atan";
const char string142[] = "sinh";
const char string143[] = "cosh";
const char string144[] = "tanh";
const char string145[] = "exp";
const char string146[] = "sqrt";
const char string147[] = "log";
const char string148[] = "expt";
const char string149[] = "ceiling";
const char string150[] = "floor";
const char string151[] = "truncate";
const char string152[] = "round";
const char string153[] = "char-code";
const char string154[] = "code-char";
const char string155[] = "characterp";
const char string156[] = "stringp";
const char string157[] = "string=";
const char string158[] = "string<";
const char string159[] = "string>";
const char string160[] = "string/=";
const char string161[] = "string<=";
const char string162[] = "string>=";
const char string163[] = "sort";
const char string164[] = "concatenate";
const char string165[] = "subseq";
const char string166[] = "search";
const char string167[] = "read-from-string";
const char string168[] = "princ-to-string";
const char string169[] = "prin1-to-string";
const char string170[] = "logand";
const char string171[] = "logior";
const char string172[] = "logxor";
const char string173[] = "lognot";
const char string174[] = "ash";
const char string175[] = "logbitp";
const char string176[] = "eval";
const char string177[] = "return";
const char string178[] = "globals";
const char string179[] = "locals";
const char string180[] = "makunbound";
const char string181[] = "break";
const char string182[] = "read";
const char string183[] = "prin1";
const char string184[] = "print";
const char string185[] = "princ";
const char string186[] = "terpri";
const char string187[] = "read-byte";
const char string188[] = "read-line";
const char string189[] = "write-byte";
const char string190[] = "write-string";
const char string191[] = "write-line";
const char string192[] = "restart-i2c";
const char string193[] = "gc";
const char string194[] = "room";
const char string195[] = "backtrace";
const char string196[] = "save-image";
const char string197[] = "load-image";
const char string198[] = "cls";
const char string199[] = "digitalread";
const char string200[] = "analogreadresolution";
const char string201[] = "analogwrite";
const char string202[] = "analogwriteresolution";
const char string203[] = "delay";
const char string204[] = "millis";
const char string205[] = "sleep";
const char string206[] = "note";
const char string207[] = "edit";
const char string208[] = "pprint";
const char string209[] = "pprintall";
const char string210[] = "require";
const char string211[] = "list-library";
const char string212[] = "?";
const char string213[] = "documentation";
const char string214[] = "apropos";
const char string215[] = "apropos-list";
const char string216[] = "unwind-protect";
const char string217[] = "ignore-errors";
const char string218[] = "error";
const char string219[] = "directory";
const char string220[] = "with-client";
const char string221[] = "available";
const char string222[] = "wifi-server";
const char string223[] = "wifi-softap";
const char string224[] = "connected";
const char string225[] = "wifi-localip";
const char string226[] = "wifi-connect";
const char string227[] = "with-gfx";
const char string228[] = "draw-pixel";
const char string229[] = "draw-line";
const char string230[] = "draw-rect";
const char string231[] = "fill-rect";
const char string232[] = "draw-circle";
const char string233[] = "fill-circle";
const char string234[] = "draw-round-rect";
const char string235[] = "fill-round-rect";
const char string236[] = "draw-triangle";
const char string237[] = "fill-triangle";
const char string238[] = "draw-char";
const char string239[] = "set-cursor";
const char string240[] = "set-text-color";
const char string241[] = "set-text-size";
const char string242[] = "set-text-wrap";
const char string243[] = "fill-screen";
const char string244[] = "set-rotation";
const char string245[] = "invert-display";
const char string246[] = "get-key";
const char string247[] = "read-pixel";
const char string248[] = "save-bmp";
const char string249[] = ":led-builtin";
const char string250[] = ":high";
const char string251[] = ":low";
const char string252[] = ":input";
const char string253[] = ":input-pullup";
const char string254[] = ":input-pulldown";
const char string255[] = ":output";
const char string256[] = ":gpio-in";
const char string257[] = ":gpio-out";
const char string258[] = ":gpio-out-set";
const char string259[] = ":gpio-out-clr";
const char string260[] = ":gpio-out-xor";
const char string261[] = ":gpio-oe";
const char string262[] = ":gpio-oe-set";
const char string263[] = ":gpio-oe-clr";
const char string264[] = ":gpio-oe-xor";

// Documentation strings
const char doc0[] = "nil\n"
"A symbol equivalent to the empty list (). Also represents false.";
const char doc1[] = "t\n"
"A symbol representing true.";
const char doc2[] = "nothing\n"
"A symbol with no value.\n"
"It is useful if you want to suppress printing the result of evaluating a function.";
const char doc3[] = "&optional\n"
"Can be followed by one or more optional parameters in a lambda or defun parameter list.";
const char doc4[] = "*features*\n"
"Returns a list of keywords representing features supported by this platform.";
const char doc12[] = "&rest\n"
"Can be followed by a parameter in a lambda or defun parameter list,\n"
"and is assigned a list of the corresponding arguments.";
const char doc13[] = "(lambda (parameter*) form*)\n"
"Creates an unnamed function with parameters. The body is evaluated with the parameters as local variables\n"
"whose initial values are defined by the values of the forms after the lambda form.";
const char doc14[] = "(let ((var value) ... ) forms*)\n"
"Declares local variables with values, and evaluates the forms with those local variables.";
const char doc15[] = "(let* ((var value) ... ) forms*)\n"
"Declares local variables with values, and evaluates the forms with those local variables.\n"
"Each declaration can refer to local variables that have been defined earlier in the let*.";
const char doc19[] = "(defun name (parameters) form*)\n"
"Defines a function.";
const char doc20[] = "(defvar variable form)\n"
"Defines a global variable.";
const char doc21[] = "(defcode name (parameters) form*)\n"
"Creates a machine-code function called name from a series of 16-bit integers given in the body of the form.\n"
"These are written into RAM, and can be executed by calling the function in the same way as a normal Lisp function.";
const char doc22[] = "(eq item item)\n"
"Tests whether the two arguments are the same symbol, same character, equal numbers,\n"
"or point to the same cons, and returns t or nil as appropriate.";
const char doc23[] = "(car list)\n"
"Returns the first item in a list.";
const char doc24[] = "(first list)\n"
"Returns the first item in a list. Equivalent to car.";
const char doc25[] = "(cdr list)\n"
"Returns a list with the first item removed.";
const char doc26[] = "(rest list)\n"
"Returns a list with the first item removed. Equivalent to cdr.";
const char doc27[] = "(nth number list)\n"
"Returns the nth item in list, counting from zero.";
const char doc28[] = "(aref array index [index*])\n"
"Returns an element from the specified array.";
const char doc29[] = "(char string n)\n"
"Returns the nth character in a string, counting from zero.";
const char doc30[] = "(string item)\n"
"Converts its argument to a string.";
const char doc31[] = "(pinmode pin mode)\n"
"Sets the input/output mode of an Arduino pin number, and returns nil.\n"
"The mode parameter can be an integer, a keyword, or t or nil.";
const char doc32[] = "(digitalwrite pin state)\n"
"Sets the state of the specified Arduino pin number.";
const char doc33[] = "(analogread pin)\n"
"Reads the specified Arduino analogue pin number and returns the value.";
const char doc34[] = "(analogreference keyword)\n"
"Specifies a keyword to set the analogue reference voltage used for analogue input.";
const char doc35[] = "(register address [value])\n"
"Reads or writes the value of a peripheral register.\n"
"If value is not specified the function returns the value of the register at address.\n"
"If value is specified the value is written to the register at address and the function returns value.";
const char doc36[] = "(format output controlstring [arguments]*)\n"
"Outputs its arguments formatted according to the format directives in controlstring.";
const char doc37[] = "(or item*)\n"
"Evaluates its arguments until one returns non-nil, and returns its value.";
const char doc38[] = "(setq symbol value [symbol value]*)\n"
"For each pair of arguments assigns the value of the second argument\n"
"to the variable specified in the first argument.";
const char doc39[] = "(loop forms*)\n"
"Executes its arguments repeatedly until one of the arguments calls (return),\n"
"which then causes an exit from the loop.";
const char doc40[] = "(push item place)\n"
"Modifies the value of place, which should be a list, to add item onto the front of the list,\n"
"and returns the new list.";
const char doc41[] = "(pop place)\n"
"Modifies the value of place, which should be a non-nil list, to remove its first item,\n"
"and returns that item.";
const char doc42[] = "(incf place [number])\n"
"Increments a place, which should have an numeric value, and returns the result.\n"
"The third argument is an optional increment which defaults to 1.";
const char doc43[] = "(decf place [number])\n"
"Decrements a place, which should have an numeric value, and returns the result.\n"
"The third argument is an optional decrement which defaults to 1.";
const char doc44[] = "(setf place value [place value]*)\n"
"For each pair of arguments modifies a place to the result of evaluating value.";
const char doc45[] = "(dolist (var list [result]) form*)\n"
"Sets the local variable var to each element of list in turn, and executes the forms.\n"
"It then returns result, or nil if result is omitted.";
const char doc46[] = "(dotimes (var number [result]) form*)\n"
"Executes the forms number times, with the local variable var set to each integer from 0 to number-1 in turn.\n"
"It then returns result, or nil if result is omitted.";
const char doc47[] = "(do ((var [init [step]])*) (end-test result*) form*)\n"
"Accepts an arbitrary number of iteration vars, which are initialised to init and stepped by step sequentially.\n"
"The forms are executed until end-test is true. It returns result.";
const char doc48[] = "(do* ((var [init [step]])*) (end-test result*) form*)\n"
"Accepts an arbitrary number of iteration vars, which are initialised to init and stepped by step in parallel.\n"
"The forms are executed until end-test is true. It returns result.";
const char doc49[] = "(trace [function]*)\n"
"Turns on tracing of up to TRACEMAX user-defined functions,\n"
"and returns a list of the functions currently being traced.";
const char doc50[] = "(untrace [function]*)\n"
"Turns off tracing of up to TRACEMAX user-defined functions, and returns a list of the functions untraced.\n"
"If no functions are specified it untraces all functions.";
const char doc51[] = "(for-millis ([number]) form*)\n"
"Executes the forms and then waits until a total of number milliseconds have elapsed.\n"
"Returns the total number of milliseconds taken.";
const char doc52[] = "(time form)\n"
"Prints the value returned by the form, and the time taken to evaluate the form\n"
"in milliseconds or seconds.";
const char doc53[] = "(with-output-to-string (str) form*)\n"
"Returns a string containing the output to the stream variable str.";
const char doc54[] = "(with-serial (str port [baud]) form*)\n"
"Evaluates the forms with str bound to a serial-stream using port.\n"
"The optional baud gives the baud rate divided by 100, default 96.";
const char doc55[] = "(with-i2c (str [port] address [read-p]) form*)\n"
"Evaluates the forms with str bound to an i2c-stream defined by address.\n"
"If read-p is nil or omitted the stream is written to, otherwise it specifies the number of bytes\n"
"to be read from the stream. If port is omitted it defaults to 0, otherwise it specifies the port, 0 or 1.";
const char doc56[] = "(with-spi (str pin [clock] [bitorder] [mode] [port]) form*)\n"
"Evaluates the forms with str bound to an spi-stream.\n"
"The parameters specify the enable pin, clock in kHz (default 4000),\n"
"bitorder 0 for LSBFIRST and 1 for MSBFIRST (default 1), SPI mode (default 0), and port 0 or 1 (default 0).";
const char doc57[] = "(with-sd-card (str filename [mode]) form*)\n"
"Evaluates the forms with str bound to an sd-stream reading from or writing to the file filename.\n"
"If mode is omitted the file is read, otherwise 0 means read, 1 write-append, or 2 write-overwrite.";
const char doc58[] = "(progn form*)\n"
"Evaluates several forms grouped together into a block, and returns the result of evaluating the last form.";
const char doc59[] = "(if test then [else])\n"
"Evaluates test. If it's non-nil the form then is evaluated and returned;\n"
"otherwise the form else is evaluated and returned.";
const char doc60[] = "(cond ((test form*) (test form*) ... ))\n"
"Each argument is a list consisting of a test optionally followed by one or more forms.\n"
"If the test evaluates to non-nil the forms are evaluated, and the last value is returned as the result of the cond.\n"
"If the test evaluates to nil, none of the forms are evaluated, and the next argument is processed in the same way.";
const char doc61[] = "(when test form*)\n"
"Evaluates the test. If it's non-nil the forms are evaluated and the last value is returned.";
const char doc62[] = "(unless test form*)\n"
"Evaluates the test. If it's nil the forms are evaluated and the last value is returned.";
const char doc63[] = "(case keyform ((key form*) (key form*) ... ))\n"
"Evaluates a keyform to produce a test key, and then tests this against a series of arguments,\n"
"each of which is a list containing a key optionally followed by one or more forms.";
const char doc64[] = "(and item*)\n"
"Evaluates its arguments until one returns nil, and returns the last value.";
const char doc65[] = "(not item)\n"
"Returns t if its argument is nil, or nil otherwise. Equivalent to null.";
const char doc66[] = "(null list)\n"
"Returns t if its argument is nil, or nil otherwise. Equivalent to not.";
const char doc67[] = "(cons item item)\n"
"If the second argument is a list, cons returns a new list with item added to the front of the list.\n"
"If the second argument isn't a list cons returns a dotted pair.";
const char doc68[] = "(atom item)\n"
"Returns t if its argument is a single number, symbol, or nil.";
const char doc69[] = "(listp item)\n"
"Returns t if its argument is a list.";
const char doc70[] = "(consp item)\n"
"Returns t if its argument is a non-null list.";
const char doc71[] = "(symbolp item)\n"
"Returns t if its argument is a symbol.";
const char doc72[] = "(arrayp item)\n"
"Returns t if its argument is an array.";
const char doc73[] = "(boundp item)\n"
"Returns t if its argument is a symbol with a value.";
const char doc74[] = "(keywordp item)\n"
"Returns non-nil if its argument is a built-in or user-defined keyword.";
const char doc75[] = "(set symbol value [symbol value]*)\n"
"For each pair of arguments, assigns the value of the second argument to the value of the first argument.";
const char doc76[] = "(streamp item)\n"
"Returns t if its argument is a stream.";
const char doc77[] = "(equal item item)\n"
"Tests whether the two arguments are the same symbol, same character, equal numbers,\n"
"or point to the same cons, and returns t or nil as appropriate.";
const char doc78[] = "(caar list)";
const char doc79[] = "(cadr list)";
const char doc80[] = "(second list)\n"
"Returns the second item in a list. Equivalent to cadr.";
const char doc81[] = "(cdar list)\n"
"Equivalent to (cdr (car list)).";
const char doc82[] = "(cddr list)\n"
"Equivalent to (cdr (cdr list)).";
const char doc83[] = "(caaar list)\n"
"Equivalent to (car (car (car list))).";
const char doc84[] = "(caadr list)\n"
"Equivalent to (car (car (cdar list))).";
const char doc85[] = "(cadar list)\n"
"Equivalent to (car (cdr (car list))).";
const char doc86[] = "(caddr list)\n"
"Equivalent to (car (cdr (cdr list))).";
const char doc87[] = "(third list)\n"
"Returns the third item in a list. Equivalent to caddr.";
const char doc88[] = "(cdaar list)\n"
"Equivalent to (cdar (car (car list))).";
const char doc89[] = "(cdadr list)\n"
"Equivalent to (cdr (car (cdr list))).";
const char doc90[] = "(cddar list)\n"
"Equivalent to (cdr (cdr (car list))).";
const char doc91[] = "(cdddr list)\n"
"Equivalent to (cdr (cdr (cdr list))).";
const char doc92[] = "(length item)\n"
"Returns the number of items in a list, the length of a string, or the length of a one-dimensional array.";
const char doc93[] = "(array-dimensions item)\n"
"Returns a list of the dimensions of an array.";
const char doc94[] = "(list item*)\n"
"Returns a list of the values of its arguments.";
const char doc95[] = "(copy-list list)\n"
"Returns a copy of a list.";
const char doc96[] = "(make-array size [:initial-element element] [:element-type 'bit])\n"
"If size is an integer it creates a one-dimensional array with elements from 0 to size-1.\n"
"If size is a list of n integers it creates an n-dimensional array with those dimensions.\n"
"If :element-type 'bit is specified the array is a bit array.";
const char doc97[] = "(reverse list)\n"
"Returns a list with the elements of list in reverse order.";
const char doc98[] = "(assoc key list [:test function])\n"
"Looks up a key in an association list of (key . value) pairs, using eq or the specified test function,\n"
"and returns the matching pair, or nil if no pair is found.";
const char doc99[] = "(member item list [:test function])\n"
"Searches for an item in a list, using eq or the specified test function, and returns the list starting\n"
"from the first occurrence of the item, or nil if it is not found.";
const char doc100[] = "(apply function list)\n"
"Returns the result of evaluating function, with the list of arguments specified by the second parameter.";
const char doc101[] = "(funcall function argument*)\n"
"Evaluates function with the specified arguments.";
const char doc102[] = "(append list*)\n"
"Joins its arguments, which should be lists, into a single list.";
const char doc103[] = "(mapc function list1 [list]*)\n"
"Applies the function to each element in one or more lists, ignoring the results.\n"
"It returns the first list argument.";
const char doc104[] = "(mapl function list1 [list]*)\n"
"Applies the function to one or more lists and then successive cdrs of those lists,\n"
"ignoring the results. It returns the first list argument.";
const char doc105[] = "(mapcar function list1 [list]*)\n"
"Applies the function to each element in one or more lists, and returns the resulting list.";
const char doc106[] = "(mapcan function list1 [list]*)\n"
"Applies the function to each element in one or more lists. The results should be lists,\n"
"and these are destructively concatenated together to give the value returned.";
const char doc107[] = "(maplist function list1 [list]*)\n"
"Applies the function to one or more lists and then successive cdrs of those lists,\n"
"and returns the resulting list.";
const char doc108[] = "(mapcon function list1 [list]*)\n"
"Applies the function to one or more lists and then successive cdrs of those lists,\n"
"and these are destructively concatenated together to give the value returned.";
const char doc109[] = "(+ number*)\n"
"Adds its arguments together.\n"
"If each argument is an integer, and the running total doesn't overflow, the result is an integer,\n"
"otherwise a floating-point number.";
const char doc110[] = "(- number*)\n"
"If there is one argument, negates the argument.\n"
"If there are two or more arguments, subtracts the second and subsequent arguments from the first argument.\n"
"If each argument is an integer, and the running total doesn't overflow, returns the result as an integer,\n"
"otherwise a floating-point number.";
const char doc111[] = "(* number*)\n"
"Multiplies its arguments together.\n"
"If each argument is an integer, and the running total doesn't overflow, the result is an integer,\n"
"otherwise it's a floating-point number.";
const char doc112[] = "(/ number*)\n"
"Divides the first argument by the second and subsequent arguments.\n"
"If each argument is an integer, and each division produces an exact result, the result is an integer;\n"
"otherwise it's a floating-point number.";
const char doc113[] = "(mod number number)\n"
"Returns its first argument modulo the second argument.\n"
"If both arguments are integers the result is an integer; otherwise it's a floating-point number.";
const char doc114[] = "(rem number number)\n"
"Returns the remainder from dividing the first argument by the second argument.\n"
"If both arguments are integers the result is an integer; otherwise it's a floating-point number.";
const char doc115[] = "(1+ number)\n"
"Adds one to its argument and returns it.\n"
"If the argument is an integer the result is an integer if possible;\n"
"otherwise it's a floating-point number.";
const char doc116[] = "(1- number)\n"
"Subtracts one from its argument and returns it.\n"
"If the argument is an integer the result is an integer if possible;\n"
"otherwise it's a floating-point number.";
const char doc117[] = "(abs number)\n"
"Returns the absolute, positive value of its argument.\n"
"If the argument is an integer the result will be returned as an integer if possible,\n"
"otherwise a floating-point number.";
const char doc118[] = "(random number)\n"
"If number is an integer returns a random number between 0 and one less than its argument.\n"
"Otherwise returns a floating-point number between zero and number.";
const char doc119[] = "(max number*)\n"
"Returns the maximum of one or more arguments.";
const char doc120[] = "(min number*)\n"
"Returns the minimum of one or more arguments.";
const char doc121[] = "(/= number*)\n"
"Returns t if none of the arguments are equal, or nil if two or more arguments are equal.";
const char doc122[] = "(= number*)\n"
"Returns t if all the arguments, which must be numbers, are numerically equal, and nil otherwise.";
const char doc123[] = "(< number*)\n"
"Returns t if each argument is less than the next argument, and nil otherwise.";
const char doc124[] = "(<= number*)\n"
"Returns t if each argument is less than or equal to the next argument, and nil otherwise.";
const char doc125[] = "(> number*)\n"
"Returns t if each argument is greater than the next argument, and nil otherwise.";
const char doc126[] = "(>= number*)\n"
"Returns t if each argument is greater than or equal to the next argument, and nil otherwise.";
const char doc127[] = "(plusp number)\n"
"Returns t if the argument is greater than zero, or nil otherwise.";
const char doc128[] = "(minusp number)\n"
"Returns t if the argument is less than zero, or nil otherwise.";
const char doc129[] = "(zerop number)\n"
"Returns t if the argument is zero.";
const char doc130[] = "(oddp number)\n"
"Returns t if the integer argument is odd.";
const char doc131[] = "(evenp number)\n"
"Returns t if the integer argument is even.";
const char doc132[] = "(integerp number)\n"
"Returns t if the argument is an integer.";
const char doc133[] = "(numberp number)\n"
"Returns t if the argument is a number.";
const char doc134[] = "(float number)\n"
"Returns its argument converted to a floating-point number.";
const char doc135[] = "(floatp number)\n"
"Returns t if the argument is a floating-point number.";
const char doc136[] = "(sin number)\n"
"Returns sin(number).";
const char doc137[] = "(cos number)\n"
"Returns cos(number).";
const char doc138[] = "(tan number)\n"
"Returns tan(number).";
const char doc139[] = "(asin number)\n"
"Returns asin(number).";
const char doc140[] = "(acos number)\n"
"Returns acos(number).";
const char doc141[] = "(atan number1 [number2])\n"
"Returns the arc tangent of number1/number2, in radians. If number2 is omitted it defaults to 1.";
const char doc142[] = "(sinh number)\n"
"Returns sinh(number).";
const char doc143[] = "(cosh number)\n"
"Returns cosh(number).";
const char doc144[] = "(tanh number)\n"
"Returns tanh(number).";
const char doc145[] = "(exp number)\n"
"Returns exp(number).";
const char doc146[] = "(sqrt number)\n"
"Returns sqrt(number).";
const char doc147[] = "(log number [base])\n"
"Returns the logarithm of number to the specified base. If base is omitted it defaults to e.";
const char doc148[] = "(expt number power)\n"
"Returns number raised to the specified power.\n"
"Returns the result as an integer if the arguments are integers and the result will be within range,\n"
"otherwise a floating-point number.";
const char doc149[] = "(ceiling number [divisor])\n"
"Returns the integer closest to +infinity for number/divisor. If divisor is omitted it defaults to 1.";
const char doc150[] = "(floor number [divisor])\n"
"Returns the integer closest to -infinity for number/divisor. If divisor is omitted it defaults to 1.";
const char doc151[] = "(truncate number [divisor])\n"
"Returns the integer part of number/divisor. If divisor is omitted it defaults to 1.";
const char doc152[] = "(round number [divisor])\n"
"Returns the integer closest to number/divisor. If divisor is omitted it defaults to 1.";
const char doc153[] = "(char-code character)\n"
"Returns the ASCII code for a character, as an integer.";
const char doc154[] = "(code-char integer)\n"
"Returns the character for the specified ASCII code.";
const char doc155[] = "(characterp item)\n"
"Returns t if the argument is a character and nil otherwise.";
const char doc156[] = "(stringp item)\n"
"Returns t if the argument is a string and nil otherwise.";
const char doc157[] = "(string= string string)\n"
"Returns t if the two strings are the same, or nil otherwise.";
const char doc158[] = "(string< string string)\n"
"Returns the index to the first mismatch if the first string is alphabetically less than the second string,\n"
"or nil otherwise.";
const char doc159[] = "(string> string string)\n"
"Returns the index to the first mismatch if the first string is alphabetically greater than the second string,\n"
"or nil otherwise.";
const char doc160[] = "(string/= string string)\n"
"Returns the index to the first mismatch if the two strings are not the same, or nil otherwise.";
const char doc161[] = "(string<= string string)\n"
"Returns the index to the first mismatch if the first string is alphabetically less than or equal to\n"
"the second string, or nil otherwise.";
const char doc162[] = "(string>= string string)\n"
"Returns the index to the first mismatch if the first string is alphabetically greater than or equal to\n"
"the second string, or nil otherwise.";
const char doc163[] = "(sort list test)\n"
"Destructively sorts list according to the test function, using an insertion sort, and returns the sorted list.";
const char doc164[] = "(concatenate 'string string*)\n"
"Joins together the strings given in the second and subsequent arguments, and returns a single string.";
const char doc165[] = "(subseq seq start [end])\n"
"Returns a subsequence of a list or string from item start to item end-1.";
const char doc166[] = "(search pattern target [:test function])\n"
"Returns the index of the first occurrence of pattern in target, or nil if it's not found.\n"
"The target can be a list or string. If it's a list a test function can be specified; default eq.";
const char doc167[] = "(read-from-string string)\n"
"Reads an atom or list from the specified string and returns it.";
const char doc168[] = "(princ-to-string item)\n"
"Prints its argument to a string, and returns the string.\n"
"Characters and strings are printed without quotation marks or escape characters.";
const char doc169[] = "(prin1-to-string item [stream])\n"
"Prints its argument to a string, and returns the string.\n"
"Characters and strings are printed with quotation marks and escape characters,\n"
"in a format that will be suitable for read-from-string.";
const char doc170[] = "(logand [value*])\n"
"Returns the bitwise & of the values.";
const char doc171[] = "(logior [value*])\n"
"Returns the bitwise | of the values.";
const char doc172[] = "(logxor [value*])\n"
"Returns the bitwise ^ of the values.";
const char doc173[] = "(lognot value)\n"
"Returns the bitwise logical NOT of the value.";
const char doc174[] = "(ash value shift)\n"
"Returns the result of bitwise shifting value by shift bits. If shift is positive, value is shifted to the left.";
const char doc175[] = "(logbitp bit value)\n"
"Returns t if bit number bit in value is a '1', and nil if it is a '0'.";
const char doc176[] = "(eval form*)\n"
"Evaluates its argument an extra time.";
const char doc177[] = "(return [value])\n"
"Exits from a (dotimes ...), (dolist ...), or (loop ...) loop construct and returns value.";
const char doc178[] = "(globals)\n"
"Returns a list of global variables.";
const char doc179[] = "(locals)\n"
"Returns an association list of local variables and their values.";
const char doc180[] = "(makunbound symbol)\n"
"Removes the value of the symbol from GlobalEnv and returns the symbol.";
const char doc181[] = "(break)\n"
"Inserts a breakpoint in the program. When evaluated prints Break! and reenters the REPL.";
const char doc182[] = "(read [stream])\n"
"Reads an atom or list from the serial input and returns it.\n"
"If stream is specified the item is read from the specified stream.";
const char doc183[] = "(prin1 item [stream])\n"
"Prints its argument, and returns its value.\n"
"Strings are printed with quotation marks and escape characters.";
const char doc184[] = "(print item [stream])\n"
"Prints its argument with quotation marks and escape characters, on a new line, and followed by a space.\n"
"If stream is specified the argument is printed to the specified stream.";
const char doc185[] = "(princ item [stream])\n"
"Prints its argument, and returns its value.\n"
"Characters and strings are printed without quotation marks or escape characters.";
const char doc186[] = "(terpri [stream])\n"
"Prints a new line, and returns nil.\n"
"If stream is specified the new line is written to the specified stream.";
const char doc187[] = "(read-byte stream)\n"
"Reads a byte from a stream and returns it.";
const char doc188[] = "(read-line [stream])\n"
"Reads characters from the serial input up to a newline character, and returns them as a string, excluding the newline.\n"
"If stream is specified the line is read from the specified stream.";
const char doc189[] = "(write-byte number [stream])\n"
"Writes a byte to a stream.";
const char doc190[] = "(write-string string [stream])\n"
"Writes a string. If stream is specified the string is written to the stream.";
const char doc191[] = "(write-line string [stream])\n"
"Writes a string terminated by a newline character. If stream is specified the string is written to the stream.";
const char doc192[] = "(restart-i2c stream [read-p])\n"
"Restarts an i2c-stream.\n"
"If read-p is nil or omitted the stream is written to.\n"
"If read-p is an integer it specifies the number of bytes to be read from the stream.";
const char doc193[] = "(gc [print time])\n"
"Forces a garbage collection and prints the number of objects collected, and the time taken.";
const char doc194[] = "(room)\n"
"Returns the number of free Lisp cells remaining.";
const char doc195[] = "(backtrace [on])\n"
"Sets the state of backtrace according to the boolean flag 'on',\n"
"or with no argument displays the current state of backtrace.";
const char doc196[] = "(save-image [symbol])\n"
"Saves the current uLisp image to non-volatile memory or SD card so it can be loaded using load-image.";
const char doc197[] = "(load-image [filename])\n"
"Loads a saved uLisp image from non-volatile memory or SD card.";
const char doc198[] = "(cls)\n"
"Prints a clear-screen character.";
const char doc199[] = "(digitalread pin)\n"
"Reads the state of the specified Arduino pin number and returns t (high) or nil (low).";
const char doc200[] = "(analogreadresolution bits)\n"
"Specifies the resolution for the analogue inputs on platforms that support it.\n"
"The default resolution on all platforms is 10 bits.";
const char doc201[] = "(analogwrite pin value)\n"
"Writes the value to the specified Arduino pin number.";
const char doc202[] = "(analogwrite pin value)\n"
"Sets the analogue write resolution.";
const char doc203[] = "(delay number)\n"
"Delays for a specified number of milliseconds.";
const char doc204[] = "(millis)\n"
"Returns the time in milliseconds that uLisp has been running.";
const char doc205[] = "(sleep secs)\n"
"Puts the processor into a low-power sleep mode for secs.\n"
"Only supported on some platforms. On other platforms it does delay(1000*secs).";
const char doc206[] = "(note [pin] [note] [octave])\n"
"Generates a square wave on pin.\n"
"note represents the note in the well-tempered scale.\n"
"The argument octave can specify an octave; default 0.";
const char doc207[] = "(edit 'function)\n"
"Calls the Lisp tree editor to allow you to edit a function definition.";
const char doc208[] = "(pprint item [str])\n"
"Prints its argument, using the pretty printer, to display it formatted in a structured way.\n"
"If str is specified it prints to the specified stream. It returns no value.";
const char doc209[] = "(pprintall [str])\n"
"Pretty-prints the definition of every function and variable defined in the uLisp workspace.\n"
"If str is specified it prints to the specified stream. It returns no value.";
const char doc210[] = "(require 'symbol)\n"
"Loads the definition of a function defined with defun, or a variable defined with defvar, from the Lisp Library.\n"
"It returns t if it was loaded, or nil if the symbol is already defined or isn't defined in the Lisp Library.";
const char doc211[] = "(list-library)\n"
"Prints a list of the functions defined in the List Library.";
const char doc212[] = "(? item)\n"
"Prints the documentation string of a built-in or user-defined function.";
const char doc213[] = "(documentation 'symbol [type])\n"
"Returns the documentation string of a built-in or user-defined function. The type argument is ignored.";
const char doc214[] = "(apropos item)\n"
"Prints the user-defined and built-in functions whose names contain the specified string or symbol.";
const char doc215[] = "(apropos-list item)\n"
"Returns a list of user-defined and built-in functions whose names contain the specified string or symbol.";
const char doc216[] = "(unwind-protect form1 [forms]*)\n"
"Evaluates form1 and forms in order and returns the value of form1,\n"
"but guarantees to evaluate forms even if an error occurs in form1.";
const char doc217[] = "(ignore-errors [forms]*)\n"
"Evaluates forms ignoring errors.";
const char doc218[] = "(error controlstring [arguments]*)\n"
"Signals an error. The message is printed by format using the controlstring and arguments.";
const char doc219[] = "(directory)\n"
"Returns a list of the filenames of the files on the SD card.";
const char doc220[] = "(with-client (str [address port]) form*)\n"
"Evaluates the forms with str bound to a wifi-stream.";
const char doc221[] = "(available stream)\n"
"Returns the number of bytes available for reading from the wifi-stream, or zero if no bytes are available.";
const char doc222[] = "(wifi-server)\n"
"Starts a Wi-Fi server running. It returns nil.";
const char doc223[] = "(wifi-softap ssid [password channel hidden])\n"
"Set up a soft access point to establish a Wi-Fi network.\n"
"Returns the IP address as a string or nil if unsuccessful.";
const char doc224[] = "(connected stream)\n"
"Returns t or nil to indicate if the client on stream is connected.";
const char doc225[] = "(wifi-localip)\n"
"Returns the IP address of the local network as a string.";
const char doc226[] = "(wifi-connect [ssid pass])\n"
"Connects to the Wi-Fi network ssid using password pass. It returns the IP address as a string.";
const char doc227[] = "(with-gfx (str) form*)\n"
"Evaluates the forms with str bound to an gfx-stream so you can print text\n"
"to the graphics display using the standard uLisp print commands.";
const char doc228[] = "(draw-pixel x y [colour])\n"
"Draws a pixel at coordinates (x,y) in colour, or white if omitted.";
const char doc229[] = "(draw-line x0 y0 x1 y1 [colour])\n"
"Draws a line from (x0,y0) to (x1,y1) in colour, or white if omitted.";
const char doc230[] = "(draw-rect x y w h [colour])\n"
"Draws an outline rectangle with its top left corner at (x,y), with width w,\n"
"and with height h. The outline is drawn in colour, or white if omitted.";
const char doc231[] = "(fill-rect x y w h [colour])\n"
"Draws a filled rectangle with its top left corner at (x,y), with width w,\n"
"and with height h. The outline is drawn in colour, or white if omitted.";
const char doc232[] = "(draw-circle x y r [colour])\n"
"Draws an outline circle with its centre at (x, y) and with radius r.\n"
"The circle is drawn in colour, or white if omitted.";
const char doc233[] = "(fill-circle x y r [colour])\n"
"Draws a filled circle with its centre at (x, y) and with radius r.\n"
"The circle is drawn in colour, or white if omitted.";
const char doc234[] = "(draw-round-rect x y w h radius [colour])\n"
"Draws an outline rounded rectangle with its top left corner at (x,y), with width w,\n"
"height h, and corner radius radius. The outline is drawn in colour, or white if omitted.";
const char doc235[] = "(fill-round-rect x y w h radius [colour])\n"
"Draws a filled rounded rectangle with its top left corner at (x,y), with width w,\n"
"height h, and corner radius radius. The outline is drawn in colour, or white if omitted.";
const char doc236[] = "(draw-triangle x0 y0 x1 y1 x2 y2 [colour])\n"
"Draws an outline triangle between (x1,y1), (x2,y2), and (x3,y3).\n"
"The outline is drawn in colour, or white if omitted.";
const char doc237[] = "(fill-triangle x0 y0 x1 y1 x2 y2 [colour])\n"
"Draws a filled triangle between (x1,y1), (x2,y2), and (x3,y3).\n"
"The outline is drawn in colour, or white if omitted.";
const char doc238[] = "(draw-char x y char [colour background size])\n"
"Draws the character char with its top left corner at (x,y).\n"
"The character is drawn in a 5 x 7 pixel font in colour against background,\n"
"which default to white and black respectively.\n"
"The character can optionally be scaled by size.";
const char doc239[] = "(set-cursor x y)\n"
"Sets the start point for text plotting to (x, y).";
const char doc240[] = "(set-text-color colour [background])\n"
"Sets the text colour for text plotted using (with-gfx ...).";
const char doc241[] = "(set-text-size scale)\n"
"Scales text by the specified size, default 1.";
const char doc242[] = "(set-text-wrap boolean)\n"
"Specified whether text wraps at the right-hand edge of the display; the default is t.";
const char doc243[] = "(fill-screen [colour])\n"
"Fills or clears the screen with colour, default black.";
const char doc244[] = "(set-rotation option)\n"
"Sets the display orientation for subsequent graphics commands; values are 0, 1, 2, or 3.";
const char doc245[] = "(invert-display boolean)\n"
"Mirror-images the display.";
const char doc246[] = "(get-key)\n"
"Waits for a key press and returns it as a character.";
const char doc247[] = "(read-pixel x y)\n"
"Reads a pixel at coordinates (x,y) and returns the 5-6-5 colour value.";
const char doc248[] = "(save-bmp filename)\n"
"Saves the screen as a BMP file.";

// Built-in symbol lookup table
const tbl_entry_t lookup_table[] = {
  { string0, NULL, 0000, doc0 },
  { string1, NULL, 0000, doc1 },
  { string2, NULL, 0000, doc2 },
  { string3, NULL, 0000, doc3 },
  { string4, NULL, 0000, doc4 },
  { string5, NULL, 0000, NULL },
  { string6, NULL, 0000, NULL },
  { string7, NULL, 0000, NULL },
  { string8, NULL, 0000, NULL },
  { string9, NULL, 0000, NULL },
  { string10, NULL, 0000, NULL },
  { string11, NULL, 0000, NULL },
  { string12, NULL, 0000, doc12 },
  { string13, NULL, 0017, doc13 },
  { string14, NULL, 0017, doc14 },
  { string15, NULL, 0017, doc15 },
  { string16, NULL, 0017, NULL },
  { string17, NULL, 0007, NULL },
  { string18, sp_quote, 0311, NULL },
  { string19, sp_defun, 0327, doc19 },
  { string20, sp_defvar, 0313, doc20 },
  { string21, sp_defcode, 0307, doc21 },
  { string22, fn_eq, 0222, doc22 },
  { string23, fn_car, 0211, doc23 },
  { string24, fn_car, 0211, doc24 },
  { string25, fn_cdr, 0211, doc25 },
  { string26, fn_cdr, 0211, doc26 },
  { string27, fn_nth, 0222, doc27 },
  { string28, fn_aref, 0227, doc28 },
  { string29, fn_char, 0222, doc29 },
  { string30, fn_stringfn, 0211, doc30 },
  { string31, fn_pinmode, 0222, doc31 },
  { string32, fn_digitalwrite, 0222, doc32 },
  { string33, fn_analogread, 0211, doc33 },
  { string34, fn_analogreference, 0211, doc34 },
  { string35, fn_register, 0212, doc35 },
  { string36, fn_format, 0227, doc36 },
  { string37, sp_or, 0307, doc37 },
  { string38, sp_setq, 0327, doc38 },
  { string39, sp_loop, 0307, doc39 },
  { string40, sp_push, 0322, doc40 },
  { string41, sp_pop, 0311, doc41 },
  { string42, sp_incf, 0312, doc42 },
  { string43, sp_decf, 0312, doc43 },
  { string44, sp_setf, 0327, doc44 },
  { string45, sp_dolist, 0317, doc45 },
  { string46, sp_dotimes, 0317, doc46 },
  { string47, sp_do, 0327, doc47 },
  { string48, sp_dostar, 0317, doc48 },
  { string49, sp_trace, 0301, doc49 },
  { string50, sp_untrace, 0301, doc50 },
  { string51, sp_formillis, 0317, doc51 },
  { string52, sp_time, 0311, doc52 },
  { string53, sp_withoutputtostring, 0317, doc53 },
  { string54, sp_withserial, 0317, doc54 },
  { string55, sp_withi2c, 0317, doc55 },
  { string56, sp_withspi, 0317, doc56 },
  { string57, sp_withsdcard, 0327, doc57 },
  { string58, tf_progn, 0107, doc58 },
  { string59, tf_if, 0123, doc59 },
  { string60, tf_cond, 0107, doc60 },
  { string61, tf_when, 0117, doc61 },
  { string62, tf_unless, 0117, doc62 },
  { string63, tf_case, 0117, doc63 },
  { string64, tf_and, 0107, doc64 },
  { string65, fn_not, 0211, doc65 },
  { string66, fn_not, 0211, doc66 },
  { string67, fn_cons, 0222, doc67 },
  { string68, fn_atom, 0211, doc68 },
  { string69, fn_listp, 0211, doc69 },
  { string70, fn_consp, 0211, doc70 },
  { string71, fn_symbolp, 0211, doc71 },
  { string72, fn_arrayp, 0211, doc72 },
  { string73, fn_boundp, 0211, doc73 },
  { string74, fn_keywordp, 0211, doc74 },
  { string75, fn_setfn, 0227, doc75 },
  { string76, fn_streamp, 0211, doc76 },
  { string77, fn_equal, 0222, doc77 },
  { string78, fn_caar, 0211, doc78 },
  { string79, fn_cadr, 0211, doc79 },
  { string80, fn_cadr, 0211, doc80 },
  { string81, fn_cdar, 0211, doc81 },
  { string82, fn_cddr, 0211, doc82 },
  { string83, fn_caaar, 0211, doc83 },
  { string84, fn_caadr, 0211, doc84 },
  { string85, fn_cadar, 0211, doc85 },
  { string86, fn_caddr, 0211, doc86 },
  { string87, fn_caddr, 0211, doc87 },
  { string88, fn_cdaar, 0211, doc88 },
  { string89, fn_cdadr, 0211, doc89 },
  { string90, fn_cddar, 0211, doc90 },
  { string91, fn_cdddr, 0211, doc91 },
  { string92, fn_length, 0211, doc92 },
  { string93, fn_arraydimensions, 0211, doc93 },
  { string94, fn_list, 0207, doc94 },
  { string95, fn_copylist, 0211, doc95 },
  { string96, fn_makearray, 0215, doc96 },
  { string97, fn_reverse, 0211, doc97 },
  { string98, fn_assoc, 0224, doc98 },
  { string99, fn_member, 0224, doc99 },
  { string100, fn_apply, 0227, doc100 },
  { string101, fn_funcall, 0217, doc101 },
  { string102, fn_append, 0207, doc102 },
  { string103, fn_mapc, 0227, doc103 },
  { string104, fn_mapl, 0227, doc104 },
  { string105, fn_mapcar, 0227, doc105 },
  { string106, fn_mapcan, 0227, doc106 },
  { string107, fn_maplist, 0227, doc107 },
  { string108, fn_mapcon, 0227, doc108 },
  { string109, fn_add, 0207, doc109 },
  { string110, fn_subtract, 0217, doc110 },
  { string111, fn_multiply, 0207, doc111 },
  { string112, fn_divide, 0217, doc112 },
  { string113, fn_mod, 0222, doc113 },
  { string114, fn_rem, 0222, doc114 },
  { string115, fn_oneplus, 0211, doc115 },
  { string116, fn_oneminus, 0211, doc116 },
  { string117, fn_abs, 0211, doc117 },
  { string118, fn_random, 0211, doc118 },
  { string119, fn_maxfn, 0217, doc119 },
  { string120, fn_minfn, 0217, doc120 },
  { string121, fn_noteq, 0217, doc121 },
  { string122, fn_numeq, 0217, doc122 },
  { string123, fn_less, 0217, doc123 },
  { string124, fn_lesseq, 0217, doc124 },
  { string125, fn_greater, 0217, doc125 },
  { string126, fn_greatereq, 0217, doc126 },
  { string127, fn_plusp, 0211, doc127 },
  { string128, fn_minusp, 0211, doc128 },
  { string129, fn_zerop, 0211, doc129 },
  { string130, fn_oddp, 0211, doc130 },
  { string131, fn_evenp, 0211, doc131 },
  { string132, fn_integerp, 0211, doc132 },
  { string133, fn_numberp, 0211, doc133 },
  { string134, fn_floatfn, 0211, doc134 },
  { string135, fn_floatp, 0211, doc135 },
  { string136, fn_sin, 0211, doc136 },
  { string137, fn_cos, 0211, doc137 },
  { string138, fn_tan, 0211, doc138 },
  { string139, fn_asin, 0211, doc139 },
  { string140, fn_acos, 0211, doc140 },
  { string141, fn_atan, 0212, doc141 },
  { string142, fn_sinh, 0211, doc142 },
  { string143, fn_cosh, 0211, doc143 },
  { string144, fn_tanh, 0211, doc144 },
  { string145, fn_exp, 0211, doc145 },
  { string146, fn_sqrt, 0211, doc146 },
  { string147, fn_log, 0212, doc147 },
  { string148, fn_expt, 0222, doc148 },
  { string149, fn_ceiling, 0212, doc149 },
  { string150, fn_floor, 0212, doc150 },
  { string151, fn_truncate, 0212, doc151 },
  { string152, fn_round, 0212, doc152 },
  { string153, fn_charcode, 0211, doc153 },
  { string154, fn_codechar, 0211, doc154 },
  { string155, fn_characterp, 0211, doc155 },
  { string156, fn_stringp, 0211, doc156 },
  { string157, fn_stringeq, 0222, doc157 },
  { string158, fn_stringless, 0222, doc158 },
  { string159, fn_stringgreater, 0222, doc159 },
  { string160, fn_stringnoteq, 0222, doc160 },
  { string161, fn_stringlesseq, 0222, doc161 },
  { string162, fn_stringgreatereq, 0222, doc162 },
  { string163, fn_sort, 0222, doc163 },
  { string164, fn_concatenate, 0217, doc164 },
  { string165, fn_subseq, 0223, doc165 },
  { string166, fn_search, 0224, doc166 },
  { string167, fn_readfromstring, 0211, doc167 },
  { string168, fn_princtostring, 0211, doc168 },
  { string169, fn_prin1tostring, 0211, doc169 },
  { string170, fn_logand, 0207, doc170 },
  { string171, fn_logior, 0207, doc171 },
  { string172, fn_logxor, 0207, doc172 },
  { string173, fn_lognot, 0211, doc173 },
  { string174, fn_ash, 0222, doc174 },
  { string175, fn_logbitp, 0222, doc175 },
  { string176, fn_eval, 0211, doc176 },
  { string177, fn_return, 0201, doc177 },
  { string178, fn_globals, 0200, doc178 },
  { string179, fn_locals, 0200, doc179 },
  { string180, fn_makunbound, 0211, doc180 },
  { string181, fn_break, 0200, doc181 },
  { string182, fn_read, 0201, doc182 },
  { string183, fn_prin1, 0212, doc183 },
  { string184, fn_print, 0212, doc184 },
  { string185, fn_princ, 0212, doc185 },
  { string186, fn_terpri, 0201, doc186 },
  { string187, fn_readbyte, 0202, doc187 },
  { string188, fn_readline, 0201, doc188 },
  { string189, fn_writebyte, 0212, doc189 },
  { string190, fn_writestring, 0212, doc190 },
  { string191, fn_writeline, 0212, doc191 },
  { string192, fn_restarti2c, 0212, doc192 },
  { string193, fn_gc, 0201, doc193 },
  { string194, fn_room, 0200, doc194 },
  { string195, fn_backtrace, 0201, doc195 },
  { string196, fn_saveimage, 0201, doc196 },
  { string197, fn_loadimage, 0201, doc197 },
  { string198, fn_cls, 0200, doc198 },
  { string199, fn_digitalread, 0211, doc199 },
  { string200, fn_analogreadresolution, 0211, doc200 },
  { string201, fn_analogwrite, 0222, doc201 },
  { string202, fn_analogwriteresolution, 0211, doc202 },
  { string203, fn_delay, 0211, doc203 },
  { string204, fn_millis, 0200, doc204 },
  { string205, fn_sleep, 0201, doc205 },
  { string206, fn_note, 0203, doc206 },
  { string207, fn_edit, 0211, doc207 },
  { string208, fn_pprint, 0212, doc208 },
  { string209, fn_pprintall, 0201, doc209 },
  { string210, fn_require, 0211, doc210 },
  { string211, fn_listlibrary, 0200, doc211 },
  { string212, sp_help, 0311, doc212 },
  { string213, fn_documentation, 0212, doc213 },
  { string214, fn_apropos, 0211, doc214 },
  { string215, fn_aproposlist, 0211, doc215 },
  { string216, sp_unwindprotect, 0307, doc216 },
  { string217, sp_ignoreerrors, 0307, doc217 },
  { string218, sp_error, 0317, doc218 },
  { string219, fn_directory, 0200, doc219 },
  { string220, sp_withclient, 0317, doc220 },
  { string221, fn_available, 0211, doc221 },
  { string222, fn_wifiserver, 0200, doc222 },
  { string223, fn_wifisoftap, 0204, doc223 },
  { string224, fn_connected, 0211, doc224 },
  { string225, fn_wifilocalip, 0200, doc225 },
  { string226, fn_wificonnect, 0203, doc226 },
  { string227, sp_withgfx, 0317, doc227 },
  { string228, fn_drawpixel, 0223, doc228 },
  { string229, fn_drawline, 0245, doc229 },
  { string230, fn_drawrect, 0245, doc230 },
  { string231, fn_fillrect, 0245, doc231 },
  { string232, fn_drawcircle, 0234, doc232 },
  { string233, fn_fillcircle, 0234, doc233 },
  { string234, fn_drawroundrect, 0256, doc234 },
  { string235, fn_fillroundrect, 0256, doc235 },
  { string236, fn_drawtriangle, 0267, doc236 },
  { string237, fn_filltriangle, 0267, doc237 },
  { string238, fn_drawchar, 0236, doc238 },
  { string239, fn_setcursor, 0222, doc239 },
  { string240, fn_settextcolor, 0212, doc240 },
  { string241, fn_settextsize, 0211, doc241 },
  { string242, fn_settextwrap, 0211, doc242 },
  { string243, fn_fillscreen, 0201, doc243 },
  { string244, fn_setrotation, 0211, doc244 },
  { string245, fn_invertdisplay, 0211, doc245 },
  { string246, fn_getkey, 0200, doc246 },
  { string247, fn_readpixel, 0222, doc247 },
  { string248, fn_savebmp, 0211, doc248 },
  { string249, (fn_ptr_type)LED_BUILTIN, 0, NULL },
  { string250, (fn_ptr_type)HIGH, DIGITALWRITE, NULL },
  { string251, (fn_ptr_type)LOW, DIGITALWRITE, NULL },
  { string252, (fn_ptr_type)INPUT, PINMODE, NULL },
  { string253, (fn_ptr_type)INPUT_PULLUP, PINMODE, NULL },
  { string254, (fn_ptr_type)INPUT_PULLDOWN, PINMODE, NULL },
  { string255, (fn_ptr_type)OUTPUT, PINMODE, NULL },
  { string256, (fn_ptr_type)(SIO_BASE+SIO_GPIO_IN_OFFSET), REGISTER, NULL },
  { string257, (fn_ptr_type)(SIO_BASE+SIO_GPIO_OUT_OFFSET), REGISTER, NULL },
  { string258, (fn_ptr_type)(SIO_BASE+SIO_GPIO_OUT_SET_OFFSET), REGISTER, NULL },
  { string259, (fn_ptr_type)(SIO_BASE+SIO_GPIO_OUT_CLR_OFFSET), REGISTER, NULL },
  { string260, (fn_ptr_type)(SIO_BASE+SIO_GPIO_OUT_XOR_OFFSET), REGISTER, NULL },
  { string261, (fn_ptr_type)(SIO_BASE+SIO_GPIO_OE_OFFSET), REGISTER, NULL },
  { string262, (fn_ptr_type)(SIO_BASE+SIO_GPIO_OE_SET_OFFSET), REGISTER, NULL },
  { string263, (fn_ptr_type)(SIO_BASE+SIO_GPIO_OE_CLR_OFFSET), REGISTER, NULL },
  { string264, (fn_ptr_type)(SIO_BASE+SIO_GPIO_OE_XOR_OFFSET), REGISTER, NULL },
};

#if !defined(extensions)
// Table cross-reference functions

tbl_entry_t *tables[] = {lookup_table, NULL};
const unsigned int tablesizes[] = { arraysize(lookup_table), 0 };

const tbl_entry_t *table (int n) {
  return tables[n];
}

unsigned int tablesize (int n) {
  return tablesizes[n];
}
#endif

// Table lookup functions

builtin_t lookupbuiltin (char* c) {
  unsigned int start = tablesize(0);
  for (int n=1; n>=0; n--) {
    int entries = tablesize(n);
    for (int i=0; i<entries; i++) {
      if (strcasecmp(c, (char*)(table(n)[i].string)) == 0)
        return (builtin_t)(start + i);
    }
    start = 0;
  }
  return ENDFUNCTIONS;
}

intptr_t lookupfn (builtin_t name) {
  bool n = name<tablesize(0);
  return (intptr_t)table(n?0:1)[n?name:name-tablesize(0)].fptr;
}

uint8_t getminmax (builtin_t name) {
  bool n = name<tablesize(0);
  return table(n?0:1)[n?name:name-tablesize(0)].minmax;
}

void checkminmax (builtin_t name, int nargs) {
  if (!(name < ENDFUNCTIONS)) error2("not a builtin");
  uint8_t minmax = getminmax(name);
  if (nargs<((minmax >> 3) & 0x07)) error2(toofewargs);
  if ((minmax & 0x07) != 0x07 && nargs>(minmax & 0x07)) error2(toomanyargs);
}

char *lookupdoc (builtin_t name) {
  bool n = name<tablesize(0);
  return (char*)table(n?0:1)[n?name:name-tablesize(0)].doc;
}

bool findsubstring (char *part, builtin_t name) {
  bool n = name<tablesize(0);
  return (strstr(table(n?0:1)[n?name:name-tablesize(0)].string, part) != NULL);
}

void testescape () {
  static unsigned long n;
  if (millis()-n < 500) return;
  n = millis();
  if (Serial.available() && Serial.read() == '~') error2("escape!");
  if (pc_kbd.keyCount() > 0) {
    const PCKeyboard::KeyEvent key = pc_kbd.keyEvent();
    if (key.state == PCKeyboard::StatePress) {
      char c = key.key;
      if (c == '~' || c == KEY_ESC) error2("escape!");
    }
  }
}

bool colonp (symbol_t name) {
  if (!longnamep(name)) return false;
  object *form = (object *)name;
  if (form == NULL) return false;
  return (((form->chars)>>((sizeof(int)-1)*8) & 0xFF) == ':');
}

bool keywordp (object *obj) {
  if (!(symbolp(obj) && builtinp(obj->name))) return false;
  builtin_t name = builtin(obj->name);
  bool n = name<tablesize(0);
  const char *s = table(n?0:1)[n?name:name-tablesize(0)].string;
  char c = s[0];
  return (c == ':');
}

void backtrace (symbol_t name) {
  Backtrace[TraceTop] = (name == sym(NIL)) ? sym(LAMBDA) : name;
  TraceTop = modbacktrace(TraceTop+1);
  if (TraceStart == TraceTop) TraceStart = modbacktrace(TraceStart+1);
}

