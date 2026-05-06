#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>

#if defined(sdcardsupport)
#include <SD.h>
#endif

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_print.h"
#include "ulisp_picocalc.h"
#include "ulisp_fwd_decls.h"

extern PCKeyboard pc_kbd;
extern TFT_eSPI tft;

// PicoCalc extras

char getKey () {
  for (;;) {
    if (pc_kbd.keyCount() > 0) {
      const PCKeyboard::KeyEvent key = pc_kbd.keyEvent();
      if (key.state == PCKeyboard::StatePress) {
        char temp = key.key;
        if ((temp != 0) && (temp != 255) && (temp != 0xA1) && (temp != 0xA2) && (temp != 0xA3) && (temp != 0xA4) && (temp != 0xA5)) {
          return temp;
        }
      }
    }
  }
}

object *fn_getkey (object *args, object *env) {
  (void) env, (void) args;
  return character(getKey());
}

object *fn_readpixel (object *args, object *env) {
  (void) env;
  #if defined(gfxsupport)
  return number(tft.readPixel(checkinteger(first(args)), checkinteger(second(args))));
  #else
  (void) args;
  #endif
  return nil;
}

// Write two bytes, least significant byte first
void writeTwo (File SDfile, uint16_t word) {
  SDfile.write(word & 0xFF); SDfile.write((word >> 8) & 0xFF);
}

// Write four bytes, least significant byte first
void writeFour (File SDfile, uint32_t word) {
  SDfile.write(word & 0xFF); SDfile.write((word >> 8) & 0xFF);
  SDfile.write((word >> 16) & 0xFF); SDfile.write((word >> 24) & 0xFF);
}

void savebmp (object *arg) {
#if defined(sdcardsupport)
  uint16_t width = 320, height = 320;
  SDBegin();
  File SDfile;
  char buffer[BUFFERSIZE];
  SDfile = SD.open(MakeFilename(checkstring(arg), buffer), O_RDWR | O_CREAT | O_TRUNC);
  //
  // File header: 14 bytes
  SDfile.write('B'); SDfile.write('M');
  writeFour(SDfile, 14+40+width*height*2);    // File size in bytes
  writeFour(SDfile, 0);
  writeFour(SDfile, 14+40);                   // Offset to image data from start
  //
  // Image header: 40 bytes
  writeFour(SDfile, 40);                      // Header size
  writeFour(SDfile, width);                   // Image width
  writeFour(SDfile, height);                  // Image height
  writeTwo(SDfile, 1);                        // Planes
  writeTwo(SDfile, 16);                       // Bits per pixel
  writeFour(SDfile, 0);                       // Compression (none)
  writeFour(SDfile, 0);                       // Image size (0 for uncompressed)
  writeFour(SDfile, 0);                       // Preferred X resolution (ignore)
  writeFour(SDfile, 0);                       // Preferred Y resolution (ignore)
  writeFour(SDfile, 0);                       // Colour map entries (ignore)
  writeFour(SDfile, 0);                       // Important colours (ignore)
  //
  // Image data: width * height * 2 bytes
  for (int y=height-1; y>=0; y--) {
    for (int x=0; x<width; x++) {
      uint16_t rgb = tft.readPixel(x,y);
      writeTwo(SDfile, (rgb & 0xFFC0)>>1 | (rgb & 0x1F)); // Convert to 555 format
    }
  }
  SDfile.close();
  SD.end();
#else
  (void) arg;
  error2("not available");
  return 0;
#endif
}

object *fn_savebmp (object *args, object *env) {
  object *arg = first(args);
  savebmp(arg);
  return arg;
}

