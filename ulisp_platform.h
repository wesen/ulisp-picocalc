// ulisp_platform.h — board pin checks, basic I2C init, tone, and sleep helpers.

#ifndef ULISP_PLATFORM_H
#define ULISP_PLATFORM_H

#include <Arduino.h>
#include <Wire.h>

void I2Cinit(TwoWire *port, bool enablePullup);
void checkanalogread(int pin);
void checkanalogwrite(int pin);
void play_tone(uint freq);
void play_tone_off();
void playnote(int pin, int note, int octave);
void nonote(int pin);
void initsleep();
void doze(int secs);

#endif // ULISP_PLATFORM_H
