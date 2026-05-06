// ulisp_config.h — compile-time feature and board configuration.
//
// Keep this file next to the firmware .cpp files.  It centralizes settings
// that were previously local to the monolithic Arduino sketch so every split
// translation unit sees the same feature flags and board sizing decisions.

#ifndef ULISP_CONFIG_H
#define ULISP_CONFIG_H

// Lisp library
extern const char LispLibrary[];

// Compile options
// #define resetautorun
#define printfreespace
#define serialmonitor
// #define printgcs
#define sdcardsupport
#define gfxsupport       // Need this for PicoCalc
// #define lisplibrary
#define assemblerlist
//#define extensions

#if defined(sdcardsupport)
#define SDCARD_SS_PIN 17
#define SDSIZE 720
#else
#define SDSIZE 0
#endif

// PicoCalc LittleFS support
#define LITTLEFS
#define FS_FILE_WRITE "w"
#define FS_FILE_READ "r"

// Platform specific settings
#define WORDALIGNED __attribute__((aligned (4)))
#define BUFFERSIZE 36  // Number of bits+4
#define RAMFUNC __attribute__ ((section (".ramfunctions")))
#define MEMBANK

// These five boards are compatible with PicoCalc (H versions are supplied with
// header pins soldered in).

// RP2040 boards ***************************************************************

#if defined(ARDUINO_RASPBERRY_PI_PICO)
  #define WORKSPACESIZE (18000-SDSIZE)    /* Objects (8*bytes) — reduced to leave RAM for REPL window */
  #define CODESIZE 256                    /* Bytes */
  #define STACKDIFF 320
  #define CPU_RP2040

#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
  #define WORKSPACESIZE (15230-SDSIZE)    /* Objects (8*bytes) */
  #define CODESIZE 256                    /* Bytes */
  #define STACKDIFF 480
  #define CPU_RP2040

// RP2350 boards ***************************************************************

#elif defined(ARDUINO_RASPBERRY_PI_PICO_2)
  #if defined(__riscv)
  #define WORKSPACESIZE (42500-SDSIZE)    /* Objects (8*bytes) */
  #define STACKDIFF 580
  #else
  #define WORKSPACESIZE (47000-SDSIZE)    /* Objects (8*bytes) */
  #define STACKDIFF 520
  #endif
  #define CODESIZE 256                    /* Bytes */
  #define CPU_RP2350

#elif defined(ARDUINO_RASPBERRY_PI_PICO_2W)
  #if defined(__riscv)
  #define WORKSPACESIZE (34900-SDSIZE)    /* Objects (8*bytes) */
  #define STACKDIFF 580
  #else
  #define WORKSPACESIZE (39200-SDSIZE)    /* Objects (8*bytes) */
  #define STACKDIFF 520
  #endif
  #define CODESIZE 256                    /* Bytes */
  #define CPU_RP2350

#elif defined(ARDUINO_PIMORONI_PICO_PLUS_2)
  //#define BOARD_HAS_PSRAM               /* Uncomment to use PSRAM */
  #if defined(BOARD_HAS_PSRAM)
  #undef MEMBANK
  #define MEMBANK PSRAM
  #define WORKSPACESIZE 1000000           /* Objects (8*bytes) */
  #define STACKDIFF 580
  #elif defined(__riscv)
  #define WORKSPACESIZE (42000-SDSIZE)    /* Objects (8*bytes) */
  #define STACKDIFF 580
  #else
  #define WORKSPACESIZE (46500-SDSIZE)    /* Objects (8*bytes) */
  #define STACKDIFF 520
  #endif
  #define CODESIZE 256                    /* Bytes */
  #undef SDCARD_SS_PIN
  #define SDCARD_SS_PIN 10
  #define CPU_RP2350

#else
#error "Board not supported!"
#endif

#endif // ULISP_CONFIG_H
