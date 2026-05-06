#include <Arduino.h>
#include <Wire.h>
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_platform.h"
#include "ulisp_messages.h"
#include "ulisp_state.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_runtime.h"
#include "ulisp_streams.h"
#include "ulisp_pretty.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_print.h"
#include "ulisp_terminal.h"
#include "ulisp_builtins.h"
#include "ulisp_arduino.h"
#include "ulisp_gfx.h"
#include "ulisp_picocalc.h"
#include "ulisp_entry.h"

object *number(int n);
void error(const char *fname, object *form);

#define AUDIO_PIN_L 26
#define AUDIO_PIN_R 27

extern const char invalidpin[];

// I2C interface for up to two ports, using Arduino Wire

void I2Cinit (TwoWire *port, bool enablePullup) {
  (void) enablePullup;
  port->begin();
}

// Stream adapters and dispatch moved to ulisp_streams.cpp.

// Check pins - these are board-specific not processor-specific

void checkanalogread (int pin) {
#if defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_W) \
  || defined(ARDUINO_RASPBERRY_PI_PICO_2) || defined(ARDUINO_RASPBERRY_PI_PICO_2W) \
  || defined(ARDUINO_PIMORONI_PICO_PLUS_2)
  if (!(pin>=26 && pin<=29)) error(invalidpin, number(pin));
#endif
}

void checkanalogwrite (int pin) {
#if defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_2) \
  || defined(ARDUINO_PIMORONI_PICO_PLUS_2)
  if (!(pin>=0 && pin<=29)) error(invalidpin, number(pin));
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO_2W)
  if (!((pin>=0 && pin<=29) || pin == 32)) error(invalidpin, number(pin));
#endif
}

// Note

void play_tone (uint freq) { // duty_c between 0..10000
    gpio_set_function(AUDIO_PIN_L, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PIN_R, GPIO_FUNC_PWM);
    uint slice_l = pwm_gpio_to_slice_num(AUDIO_PIN_L);
    uint slice_r = pwm_gpio_to_slice_num(AUDIO_PIN_R);
    pwm_config config = pwm_get_default_config();
    float div = (float)clock_get_hz(clk_sys) / (freq * 10000);
    pwm_config_set_clkdiv(&config, div);
    pwm_config_set_wrap(&config, 10000); 
    pwm_init(slice_l, &config, true); // start the pwm running according to the config
    pwm_init(slice_r, &config, true);
    pwm_set_gpio_level(AUDIO_PIN_L, 5000); //connect the pin to the pwm engine and set the duty cycle.
    pwm_set_gpio_level(AUDIO_PIN_R, 5000);
  };

void play_tone_off() {
    uint slice_l = pwm_gpio_to_slice_num(AUDIO_PIN_L);
    uint slice_r = pwm_gpio_to_slice_num(AUDIO_PIN_R);
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_l, &config, false);
    pwm_init(slice_r, &config, false);
};

const int scale[] = {4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902};

void playnote (int pin, int note, int octave) {
  (void) pin;
  int oct = octave + note/12;
  int prescaler = 8 - oct;
  if (prescaler<0 || prescaler>8) error("octave out of range", number(oct));
  play_tone(scale[note%12]>>prescaler);
}

void nonote (int pin) {
  (void) pin;
  play_tone_off();
}

// Sleep

void initsleep () { }

void doze (int secs) {
  delay(1000 * secs);
}

