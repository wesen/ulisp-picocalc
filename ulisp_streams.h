// ulisp_streams.h — stream dispatch, serial/I2C/SPI/SD/WiFi/GFX adapters.

#ifndef ULISP_STREAMS_H
#define ULISP_STREAMS_H

#include <Arduino.h>
#include <Wire.h>
#include "ulisp_types.h"

#if defined(sdcardsupport)
#include <SD.h>
extern File SDpfile;
extern File SDgfile;
#endif

enum stream { SERIALSTREAM, I2CSTREAM, SPISTREAM, SDSTREAM, WIFISTREAM, STRINGSTREAM, GFXSTREAM };

int I2Cread(TwoWire *port);
void I2Cwrite(TwoWire *port, uint8_t data);
bool I2Cstart(TwoWire *port, uint8_t address, uint8_t read);
bool I2Crestart(TwoWire *port, uint8_t address, uint8_t read);
void I2Cstop(TwoWire *port, uint8_t read);

void serialbegin(int address, int baud);
void serialend(int address);

#if defined(gfxsupport)
void gfxwrite(char c);
#endif

const stream_entry_t *streamtable(int n);
pfun_t pstreamfun(object *args);
gfun_t gstreamfun(object *args);

#endif // ULISP_STREAMS_H
