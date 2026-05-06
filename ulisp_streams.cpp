#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "ulisp_config.h"

#if defined(sdcardsupport)
#include <SD.h>
#endif

#if defined(gfxsupport)
#include <TFT_eSPI.h>
#endif

#include "ulisp_types.h"
#include "ulisp_state.h"
#include "ulisp_streams.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_arduino.h"
#include "ulisp_gfx.h"
#include "ulisp_picocalc.h"
#include "ulisp_messages.h"
#include "ulisp_runtime.h"
#include "ulisp_platform.h"
#include "ulisp_pretty.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_print.h"
#include "ulisp_terminal.h"
#include "ulisp_builtins.h"
#include "ulisp_entry.h"

#if defined(gfxsupport)
extern TFT_eSPI tft;
#endif

int I2Cread (TwoWire *port) {
  return port->read();
}

void I2Cwrite (TwoWire *port, uint8_t data) {
  port->write(data);
}

bool I2Cstart (TwoWire *port, uint8_t address, uint8_t read) {
 int ok = true;
 if (read == 0) {
   port->beginTransmission(address);
   ok = (port->endTransmission(true) == 0);
   port->beginTransmission(address);
 }
 else port->requestFrom(address, I2Ccount);
 return ok;
}

bool I2Crestart (TwoWire *port, uint8_t address, uint8_t read) {
  int error = (port->endTransmission(false) != 0);
  if (read == 0) port->beginTransmission(address);
  else port->requestFrom(address, I2Ccount);
  return error ? false : true;
}

void I2Cstop (TwoWire *port, uint8_t read) {
  if (read == 0) port->endTransmission(); // Check for error?
  // Release pins
  port->end();
}

// Streams

// Simplify board differences - default is one of each port
#if defined(ARDUINO_NRF52840_CLUE) || defined(ARDUINO_GRAND_CENTRAL_M4) \
  || defined(ARDUINO_PYBADGE_M4) || defined(ARDUINO_PYGAMER_M4) || defined(ARDUINO_TEENSY40) \
  || defined(ARDUINO_TEENSY41) || defined(ARDUINO_RASPBERRY_PI_PICO) \
  || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO_2) \
  || defined(ARDUINO_RASPBERRY_PI_PICO_2W) || defined(ARDUINO_PIMORONI_PICO_PLUS_2)
#define ULISP_HOWMANYSPI 2
#endif
#if defined(ARDUINO_WIO_TERMINAL) || defined(ARDUINO_BBC_MICROBIT_V2) \
  || defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41) || defined(MAX32620) \
  || defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_W) \
  || defined(ARDUINO_ADAFRUIT_QTPY_RP2040) || defined(ARDUINO_ADAFRUIT_FEATHER_RP2040) \
  || defined(ARDUINO_RASPBERRY_PI_PICO_2) || defined(ARDUINO_RASPBERRY_PI_PICO_2W) \
  || defined(ARDUINO_PIMORONI_PICO_PLUS_2) || defined(ARDUINO_GRAND_CENTRAL_M4) \
  || defined(ARDUINO_NRF52840_CIRCUITPLAY)
#define ULISP_HOWMANYI2C 2
#endif
#if defined(ARDUINO_SAM_DUE) || defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
#define ULISP_HOWMANYSERIAL 4
#elif defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_W) \
  || defined(ARDUINO_RASPBERRY_PI_PICO_2) || defined(ARDUINO_RASPBERRY_PI_PICO_2W) \
  || defined(ARDUINO_PIMORONI_PICO_PLUS_2)
#define ULISP_HOWMANYSERIAL 3
#elif !defined(CPU_NRF51822) && !defined(CPU_NRF52833) && !defined(ARDUINO_FEATHER_F405) \
  && !defined(ARDUINO_PIXELTRINKEY_M0)
#define ULISP_HOWMANYSERIAL 2
#endif

void spiwrite (char c) { SPI.transfer(c); }
#if ULISP_HOWMANYSPI == 2
void spi1write (char c) { SPI1.transfer(c); }
#endif
void i2cwrite (char c) { I2Cwrite(&Wire, c); }
#if ULISP_HOWMANYI2C == 2
void i2c1write (char c) { I2Cwrite(&Wire1, c); }
#endif
#if ULISP_HOWMANYSERIAL == 4
void serial1write (char c) { Serial1.write(c); }
void serial2write (char c) { Serial2.write(c); }
void serial3write (char c) { Serial3.write(c); }
#elif ULISP_HOWMANYSERIAL == 3
void serial2write (char c) { Serial2.write(c); }
void serial1write (char c) { Serial1.write(c); }
#elif ULISP_HOWMANYSERIAL == 2
void serial1write (char c) { Serial1.write(c); }
#endif
#if defined(sdcardsupport)
File SDpfile, SDgfile;
void SDwrite (char c) { SDpfile.write(uint8_t(c)); } // Fix for RP2040
#endif
#if defined(ULISP_WIFI)
WiFiClient client;
WiFiServer server(80);
void WiFiwrite (char c) { client.write(c); }
#endif
#if defined(gfxsupport)
void gfxwrite (char c) { tft.write(c); }
#endif

int spiread () { return SPI.transfer(0); }
#if ULISP_HOWMANYSPI == 2
int spi1read () { return SPI1.transfer(0); }
#endif
int i2cread () { return I2Cread(&Wire); }
#if ULISP_HOWMANYI2C == 2
int i2c1read () { return I2Cread(&Wire1); }
#endif
#if ULISP_HOWMANYSERIAL == 4
int serial3read () { while (!Serial3.available()) testescape(); return Serial3.read(); }
#endif
#if ULISP_HOWMANYSERIAL == 4 || ULISP_HOWMANYSERIAL == 3
int serial2read () { while (!Serial2.available()) testescape(); return Serial2.read(); }
#endif
#if ULISP_HOWMANYSERIAL == 4 || ULISP_HOWMANYSERIAL == 3 || ULISP_HOWMANYSERIAL == 2
int serial1read () { while (!Serial1.available()) testescape(); return Serial1.read(); }
#endif
#if defined(sdcardsupport)
int SDread () { return SDgfile.read(); }
#endif

#if defined(ULISP_WIFI)
int WiFiread () { while (!client.available()) testescape(); return client.read(); }
#endif

void serialbegin (int address, int baud) {
  #if ULISP_HOWMANYSERIAL == 4
  if (address == 1) Serial1.begin((long)baud*100);
  else if (address == 2) Serial2.begin((long)baud*100);
  else if (address == 3) Serial3.begin((long)baud*100);
  #elif ULISP_HOWMANYSERIAL == 3
  if (address == 1) Serial1.begin((long)baud*100);
  else if (address == 2) Serial2.begin((long)baud*100);
  #elif ULISP_HOWMANYSERIAL == 2
  if (address == 1) Serial1.begin((long)baud*100);
  #else
  (void) baud;
  if (false);
  #endif
  else error("port not supported", number(address));
}

void serialend (int address) {
  #if ULISP_HOWMANYSERIAL == 4
  if (address == 1) {Serial1.flush(); Serial1.end(); }
  else if (address == 2) {Serial2.flush(); Serial2.end(); }
  else if (address == 3) {Serial3.flush(); Serial3.end(); }
  #elif ULISP_HOWMANYSERIAL == 3
  if (address == 1) {Serial1.flush(); Serial1.end(); }
  else if (address == 2) {Serial2.flush(); Serial2.end(); }
  #elif ULISP_HOWMANYSERIAL == 2
  if (address == 1) {Serial1.flush(); Serial1.end(); }
  #else
  if (false);
  #endif
  else error("port not supported", number(address));
}

// Stream writing functions

pfun_t pfun_i2c (uint8_t address) {
  pfun_t pfun;
  if (address < 128) pfun = i2cwrite;
  #if ULISP_HOWMANYI2C == 2
  else pfun = i2c1write;
  #endif
  return pfun;
}

pfun_t pfun_spi (uint8_t address) {
  pfun_t pfun;
  if (address < 128) pfun = spiwrite;
  #if ULISP_HOWMANYSPI == 2
  else pfun = spi1write;
  #endif
  return pfun;
}

pfun_t pfun_serial (uint8_t address) {
  pfun_t pfun = pserial;
  if (address == 0) pfun = pserial;
  #if ULISP_HOWMANYSERIAL == 4
  else if (address == 1) pfun = serial1write;
  else if (address == 2) pfun = serial2write;
  else if (address == 3) pfun = serial3write;
  #elif ULISP_HOWMANYSERIAL == 3
  else if (address == 1) pfun = serial1write;
  else if (address == 2) pfun = serial2write;
  #elif ULISP_HOWMANYSERIAL == 2
  else if (address == 1) pfun = serial1write;
  #endif
  return pfun;
}

pfun_t pfun_string (uint8_t address) {
  (void) address;
  return pstr;
}

pfun_t pfun_sd (uint8_t address) {
  (void) address;
  pfun_t pfun = pserial;
  #if defined(sdcardsupport)
  pfun = (pfun_t)SDwrite;
  #endif
  return pfun;
}

pfun_t pfun_gfx (uint8_t address) {
  (void) address;
  pfun_t pfun = pserial;
  #if defined(gfxsupport)
  pfun = (pfun_t)gfxwrite;
  #endif
  return pfun;
}

pfun_t pfun_wifi (uint8_t address) {
  (void) address; 
  pfun_t pfun = pserial;
  #if defined(ULISP_WIFI)
  pfun = (pfun_t)WiFiwrite;
  #endif
  return pfun;
}

// Stream reading functions

gfun_t gfun_i2c (uint8_t address) {
  gfun_t gfun;
  if (address < 128) gfun = i2cread;
  #if ULISP_HOWMANYI2C == 2
  else gfun = i2c1read;
  #endif
  return gfun;
}

gfun_t gfun_spi (uint8_t address) {
  gfun_t gfun;
  if (address < 128) gfun = spiread;
  #if ULISP_HOWMANYSPI == 2
  else gfun = spi1read;
  #endif
  return gfun;
}

gfun_t gfun_serial (uint8_t address) {
  gfun_t gfun = gserial;
  if (address == 0) gfun = gserial;
  #if ULISP_HOWMANYSERIAL == 4
  else if (address == 1) gfun = serial1read;
  else if (address == 2) gfun = serial2read;
  else if (address == 3) gfun = serial3read;
  #elif ULISP_HOWMANYSERIAL == 3
  else if (address == 1) gfun = serial1read;
  else if (address == 2) gfun = serial2read;
  #elif ULISP_HOWMANYSERIAL == 2
  else if (address == 1) gfun = serial1read;
  #endif
  return gfun;
}

gfun_t gfun_sd (uint8_t address) {
  (void) address;
  gfun_t gfun = gserial;
  #if defined(sdcardsupport)
  gfun = (gfun_t)SDread;
  #endif
  return gfun;
}

gfun_t gfun_wifi (uint8_t address) {
  (void) address; 
  gfun_t gfun = gserial;
  #if defined(ULISP_WIFI)
  gfun = (gfun_t)WiFiread;
  #endif
  return gfun;
}

// Stream names used by printobject
const char serialstreamname[] = "serial";
const char i2cstreamname[] = "i2c";
const char spistreamname[] = "spi";
const char sdstreamname[] = "sd";
const char wifistreamname[] = "wifi";
const char stringstreamname[] = "string";
const char gfxstreamname[] = "gfx";

// Stream lookup table - needs to be in same order as enum stream
const stream_entry_t stream_table[] = {
  { serialstreamname, pfun_serial, gfun_serial },
  { i2cstreamname, pfun_i2c, gfun_i2c },
  { spistreamname, pfun_spi, gfun_spi },
  { sdstreamname, pfun_sd, gfun_sd },
  { wifistreamname, pfun_wifi, gfun_wifi },
  { stringstreamname, pfun_string, NULL },
  { gfxstreamname, pfun_gfx, NULL },
};

#if !defined(streamextensions)
// Stream table cross-reference functions

stream_entry_t *streamtables[] = {stream_table, NULL};

const stream_entry_t *streamtable (int n) {
  return streamtables[n];
}
#endif

pfun_t pstreamfun (object *args) {
  nstream_t nstream = SERIALSTREAM;
  int address = 0;
  pfun_t pfun = pserial;
  if (args != NULL && first(args) != NULL) {
    int stream = isstream(first(args));
    nstream = stream>>8; address = stream & 0xFF;
  }
  bool n = nstream<USERSTREAMS;
  pstream_ptr_t streamfunction = streamtable(n?0:1)[n?nstream:nstream-USERSTREAMS].pfunptr;
  pfun = streamfunction(address);
  return pfun;
}

gfun_t gstreamfun (object *args) {
  nstream_t nstream = SERIALSTREAM;
  int address = 0;
  gfun_t gfun = gserial;
  if (args != NULL) {
    int stream = isstream(first(args));
    nstream = stream>>8; address = stream & 0xFF;
  }
  bool n = nstream<USERSTREAMS;
  gstream_ptr_t streamfunction = streamtable(n?0:1)[n?nstream:nstream-USERSTREAMS].gfunptr;
  gfun = streamfunction(address);
  return gfun;
}

