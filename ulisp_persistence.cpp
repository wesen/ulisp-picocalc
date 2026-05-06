// ulisp_persistence.cpp — filename helpers and image save/load backends.

#include <Arduino.h>
#include <setjmp.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <LittleFS.h>

#include "ulisp_config.h"
#include "ulisp_types.h"
#include "ulisp_messages.h"
#include "ulisp_state.h"
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
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_print.h"
#include "ulisp_terminal.h"
#include "ulisp_builtins.h"
#include "ulisp_entry.h"

extern const char invalidarg[];

// Make SD card filename

char *MakeFilename (object *arg, char *buffer) {
  int max = BUFFERSIZE-1;
  buffer[0]='/';
  int i = 1;
  do {
    char c = nthchar(arg, i-1);
    if (c == '\0') break;
    buffer[i++] = c;
  } while (i<max);
  buffer[i] = '\0';
  return buffer;
}

// Save-image and load-image

#if defined(sdcardsupport)

void SDBegin () {
  SD.begin(SDCARD_SS_PIN, (uint32_t)SPI_HALF_SPEED, SPI);
}

void SDWrite32 (File file, int data) {
  file.write(data & 0xFF); file.write(data>>8 & 0xFF);
  file.write(data>>16 & 0xFF); file.write(data>>24 & 0xFF);
}

int SDRead32 (File file) {
  uintptr_t b0 = file.read(); uintptr_t b1 = file.read();
  uintptr_t b2 = file.read(); uintptr_t b3 = file.read();
  return b0 | b1<<8 | b2<<16 | b3<<24;
}
#elif defined(LITTLEFS)
void FSWrite32 (File file, uint32_t data) {
  union { uint32_t data2; uint8_t u8[4]; };
  data2 = data;
  if (file.write(u8, 4) != 4) error2("not enough room");
}

uint32_t FSRead32 (File file) {
  union { uint32_t data; uint8_t u8[4]; };
  file.read(u8, 4);
  return data;
}
#elif defined(DATAFLASH)
// Winbond DataFlash support for Adafruit M4 Express boards
#define PAGEPROG      0x02
#define READSTATUS    0x05
#define READDATA      0x03
#define WRITEENABLE   0x06
#define BLOCK64K      0xD8
#define READID        0x90

// Arduino pins used for dataflash
#if defined(ARDUINO_ITSYBITSY_M0) || defined(ARDUINO_SAMD_FEATHER_M0_EXPRESS)
const int sck = 38, ssel = 39, mosi = 37, miso = 36;
#elif defined(EXTERNAL_FLASH_USE_QSPI)
const int sck = PIN_QSPI_SCK, ssel = PIN_QSPI_CS, mosi = PIN_QSPI_IO0, miso = PIN_QSPI_IO1;
#endif

void FlashBusy () {
  digitalWrite(ssel, 0);
  FlashWrite(READSTATUS);
  while ((FlashReadByte() & 1) != 0);
  digitalWrite(ssel, 1);
}

inline void FlashWrite (uint8_t data) {
  shiftOut(mosi, sck, MSBFIRST, data);
}

inline uint8_t FlashReadByte () {
  return shiftIn(miso, sck, MSBFIRST);
}

void FlashWriteByte (uint32_t *addr, uint8_t data) {
  // New page
  if (((*addr) & 0xFF) == 0) {
    digitalWrite(ssel, 1);
    FlashBusy();
    FlashWriteEnable();
    digitalWrite(ssel, 0);
    FlashWrite(PAGEPROG);
    FlashWrite((*addr)>>16);
    FlashWrite((*addr)>>8);
    FlashWrite(0);
  }
  FlashWrite(data);
  (*addr)++;
}

void FlashWriteEnable () {
  digitalWrite(ssel, 0);
  FlashWrite(WRITEENABLE);
  digitalWrite(ssel, 1);
}

bool FlashCheck () {
  uint8_t devID;
  digitalWrite(ssel, HIGH); pinMode(ssel, OUTPUT);
  pinMode(sck, OUTPUT);
  pinMode(mosi, OUTPUT);
  pinMode(miso, INPUT);
  digitalWrite(sck, LOW); digitalWrite(mosi, HIGH);
  digitalWrite(ssel, LOW);
  FlashWrite(READID);
  for (uint8_t i=0; i<4; i++) FlashReadByte();
  devID = FlashReadByte();
  digitalWrite(ssel, HIGH);
  return (devID >= 0x14 && devID <= 0x17); // true = found correct device
}

void FlashBeginWrite (uint32_t *addr, uint32_t bytes) {
  *addr = 0;
  uint8_t blocks = (bytes+65535)/65536;
  // Erase 64K
  for (uint8_t b=0; b<blocks; b++) {
    FlashWriteEnable();
    digitalWrite(ssel, 0);
    FlashWrite(BLOCK64K);
    FlashWrite(b); FlashWrite(0); FlashWrite(0);
    digitalWrite(ssel, 1);
    FlashBusy();
  }
}

void FlashWrite32 (uint32_t *addr, uint32_t data) {
  FlashWriteByte(addr, data & 0xFF); FlashWriteByte(addr, data>>8 & 0xFF);
  FlashWriteByte(addr, data>>16 & 0xFF); FlashWriteByte(addr, data>>24 & 0xFF);
}

inline void FlashEndWrite (uint32_t *addr) {
  (void) addr;
  digitalWrite(ssel, 1);
  FlashBusy();
}

void FlashBeginRead (uint32_t *addr) {
  *addr = 0;
  FlashBusy();
  digitalWrite(ssel, 0);
  FlashWrite(READDATA);
  FlashWrite(0); FlashWrite(0); FlashWrite(0);
}

uint32_t FlashRead32 (uint32_t *addr) {
  (void) addr;
  uint8_t b0 = FlashReadByte(); uint8_t b1 = FlashReadByte();
  uint8_t b2 = FlashReadByte(); uint8_t b3 = FlashReadByte();
  return b0 | b1<<8 | b2<<16 | b3<<24;
}

inline void FlashEndRead(uint32_t *addr) {
  (void) addr;
  digitalWrite(ssel, 1);
}

#elif defined(CPUFLASH)
// For ATSAMD21
__attribute__((__aligned__(256))) static const uint8_t flash_store[FLASHSIZE] = { };

void row_erase (const volatile void *addr) {
  NVMCTRL->ADDR.reg = ((uint32_t)addr) / 2;
  NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
  while (!NVMCTRL->INTFLAG.bit.READY);
}

void page_clear () {
  // Execute "PBC" Page Buffer Clear
  NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
  while (NVMCTRL->INTFLAG.bit.READY == 0);
}

void page_write () {
  NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
  while (NVMCTRL->INTFLAG.bit.READY == 0);
}

bool FlashCheck() {
  return true;
}

void FlashBeginWrite(uint32_t *addr, uint32_t bytes) {
  (void) bytes;
  *addr = (uint32_t)flash_store;
  // Disable automatic page write
  NVMCTRL->CTRLB.bit.MANW = 1;
}

void FlashWrite32 (uint32_t *addr, uint32_t data) {
  if (((*addr) & 0xFF) == 0) row_erase((const volatile void *)(*addr));
  if (((*addr) & 0x3F) == 0) page_clear();
  *(volatile uint32_t *)(*addr) = data;
  (*addr) = (*addr) + 4;
  if (((*addr) & 0x3F) == 0) page_write();
}

void FlashEndWrite (uint32_t *addr) {
  if (((*addr) & 0x3F) != 0) page_write();
}

void FlashBeginRead(uint32_t *addr) {
  *addr = (uint32_t)flash_store;
}

uint32_t FlashRead32 (uint32_t *addr) {
  uint32_t data = *(volatile const uint32_t *)(*addr);
  (*addr) = (*addr) + 4;
  return data;
}

void FlashEndRead (uint32_t *addr) {
  (void) addr;
}
#elif defined(EEPROMFLASH)

bool FlashCheck() {
  return (EEPROM.length() == FLASHSIZE);
}

void FlashBeginWrite(uint32_t *addr, uint32_t bytes) {
  (void) bytes;
  *addr = 0;
}

void FlashWrite32 (uint32_t *addr, uint32_t data) {
  EEPROM.put(*addr, data);
  (*addr) = (*addr) + 4;
}

void FlashEndWrite (uint32_t *addr) {
  (void) addr;
}

void FlashBeginRead(uint32_t *addr) {
  *addr = 0;
}

uint32_t FlashRead32 (uint32_t *addr) {
  uint32_t data;
  EEPROM.get(*addr, data);
  (*addr) = (*addr) + 4;
  return data;
}

void FlashEndRead (uint32_t *addr) {
  (void) addr;
}
#endif

int saveimage (object *arg) {
#if defined(sdcardsupport)
  unsigned int imagesize = compactimage(&arg);
  SDBegin();
  File file;
  if (stringp(arg)) {
    char buffer[BUFFERSIZE];
    file = SD.open(MakeFilename(arg, buffer), O_RDWR | O_CREAT | O_TRUNC);
    if (!file) error2("problem saving to SD card or invalid filename");
    arg = NULL;
  } else if (arg == NULL || listp(arg)) {
    file = SD.open("/ULISP.IMG", O_RDWR | O_CREAT | O_TRUNC);
    if (!file) error2("problem saving to SD card");
  } else error(invalidarg, arg);
  SDWrite32(file, (uintptr_t)arg);
  SDWrite32(file, imagesize);
  SDWrite32(file, (uintptr_t)GlobalEnv);
  SDWrite32(file, (uintptr_t)GCStack);
  for (int i=0; i<CODESIZE; i++) file.write(MyCode[i]);
  for (unsigned int i=0; i<imagesize; i++) {
    object *obj = &Workspace[i];
    SDWrite32(file, (uintptr_t)car(obj));
    SDWrite32(file, (uintptr_t)cdr(obj));
  }
  file.close();
  return imagesize;
#elif defined(LITTLEFS)
  unsigned int imagesize = compactimage(&arg);
  LittleFS.begin(LITTLEFS);
  File file;
  if (stringp(arg)) {
    char buffer[BUFFERSIZE];
    file = LittleFS.open(MakeFilename(arg, buffer), FS_FILE_WRITE);
    if (!file) error2("problem saving to LittleFS or invalid filename");
    arg = NULL;
  } else if (arg == NULL || listp(arg)) {
    file = LittleFS.open("/ULISP.IMG", FS_FILE_WRITE);
    if (!file) error2("problem saving to LittleFS");
  } else error(invalidarg, arg);
  FSWrite32(file, (uintptr_t)arg);
  FSWrite32(file, imagesize);
  FSWrite32(file, (uintptr_t)GlobalEnv);
  FSWrite32(file, (uintptr_t)GCStack);
  if (file.write(MyCode, CODESIZE) != CODESIZE) error2("not enough room");
  for (unsigned int i=0; i<imagesize; i++) {
    object *obj = &Workspace[i];
    FSWrite32(file, (uintptr_t)car(obj));
    FSWrite32(file, (uintptr_t)cdr(obj));
  }
  file.close();
  return imagesize;
#elif defined(DATAFLASH) || defined(CPUFLASH) || defined(EEPROMFLASH)
  unsigned int imagesize = compactimage(&arg);
  if (!(arg == NULL || listp(arg))) error(invalidarg, arg);
  if (!FlashCheck()) error2("flash not available");
  // Save to flash
  uint32_t bytesneeded = 16 + CODESIZE + imagesize*8;
  if (bytesneeded > FLASHSIZE) error("image too large", number(imagesize));
  uint32_t addr;
  FlashBeginWrite(&addr, bytesneeded);
  FlashWrite32(&addr, (uintptr_t)arg);
  FlashWrite32(&addr, imagesize);
  FlashWrite32(&addr, (uintptr_t)GlobalEnv);
  FlashWrite32(&addr, (uintptr_t)GCStack);
  for (int i=0; i<CODESIZE; i=i+4) {
    union { uint32_t u32; uint8_t u8[4]; };
    u8[0] = MyCode[i]; u8[1] = MyCode[i+1]; u8[2] = MyCode[i+2]; u8[3] = MyCode[i+3];
    FlashWrite32(&addr, u32);
  }
  for (unsigned int i=0; i<imagesize; i++) {
    object *obj = &Workspace[i];
    FlashWrite32(&addr, (uintptr_t)car(obj));
    FlashWrite32(&addr, (uintptr_t)cdr(obj));
  }
  FlashEndWrite(&addr);
  return imagesize;
#else
  (void) arg;
  error2("not available");
  return 0;
#endif
}

int loadimage (object *arg) {
#if defined(sdcardsupport)
  SDBegin();
  File file;
  if (stringp(arg)) {
    char buffer[BUFFERSIZE];
    file = SD.open(MakeFilename(arg, buffer));
    if (!file) error2("problem loading from SD card or invalid filename");
  } else if (arg == NULL) {
    file = SD.open("/ULISP.IMG");
    if (!file) error2("problem loading from SD card");
  } else error(invalidarg, arg);
  SDRead32(file);
  unsigned int imagesize = SDRead32(file);
  GlobalEnv = (object *)SDRead32(file);
  GCStack = (object *)SDRead32(file);
  for (int i=0; i<CODESIZE; i++) MyCode[i] = file.read();
  for (unsigned int i=0; i<imagesize; i++) {
    object *obj = &Workspace[i];
    car(obj) = (object *)SDRead32(file);
    cdr(obj) = (object *)SDRead32(file);
  }
  file.close();
  gc(NULL, NULL);
  return imagesize;
#elif defined(LITTLEFS)
  LittleFS.begin(LITTLEFS);
  File file;
  if (stringp(arg)) {
    char buffer[BUFFERSIZE];
    file = LittleFS.open(MakeFilename(arg, buffer), FS_FILE_READ);
    if (!file) error2("problem loading from LittleFS or invalid filename");
  }
  else if (arg == NULL) {
    file = LittleFS.open("/ULISP.IMG", FS_FILE_READ);
    if (!file) error2("problem loading from LittleFS");
  }
  else error(invalidarg, arg);
  FSRead32(file);
  unsigned int imagesize = FSRead32(file);
  GlobalEnv = (object *)FSRead32(file);
  GCStack = (object *)FSRead32(file);
  file.read(MyCode, CODESIZE);
  for (unsigned int i=0; i<imagesize; i++) {
    object *obj = &Workspace[i];
    car(obj) = (object *)FSRead32(file);
    cdr(obj) = (object *)FSRead32(file);
  }
  file.close();
  gc(NULL, NULL);
  return imagesize;
#elif defined(DATAFLASH) || defined(CPUFLASH) || defined(EEPROMFLASH)
  (void) arg;
  if (!FlashCheck()) error2("flash not available");
  uint32_t addr;
  FlashBeginRead(&addr);
  FlashRead32(&addr); // Skip eval address
  uint32_t imagesize = FlashRead32(&addr);
  if (imagesize == 0 || imagesize == 0xFFFFFFFF) error2("no saved image");
  GlobalEnv = (object *)FlashRead32(&addr);
  GCStack = (object *)FlashRead32(&addr);
  for (int i=0; i<CODESIZE; i=i+4) {
    union { uint32_t u32; uint8_t u8[4]; };
    u32 = FlashRead32(&addr);
    MyCode[i] = u8[0]; MyCode[i+1] = u8[1]; MyCode[i+2] = u8[2]; MyCode[i+3] = u8[3];
  }
  for (uint32_t i=0; i<imagesize; i++) {
    object *obj = &Workspace[i];
    car(obj) = (object *)FlashRead32(&addr);
    cdr(obj) = (object *)FlashRead32(&addr);
  }
  FlashEndRead(&addr);
  gc(NULL, NULL);
  return imagesize;
#else
  (void) arg;
  error2("not available");
  return 0;
#endif
}

void autorunimage () {
#if defined(sdcardsupport)
  SDBegin();
  File file = SD.open("/ULISP.IMG");
  if (!file) error2("problem autorunning from SD card");
  object *autorun = (object *)SDRead32(file);
  file.close();
  if (autorun != NULL) {
    loadimage(NULL);
    apply(autorun, NULL, NULL);
  }
#elif defined(LITTLEFS)
  LittleFS.begin(LITTLEFS);
  File file = LittleFS.open("/ULISP.IMG", FS_FILE_READ);
  if (!file) error2("problem autorunning from LittleFS");
  object *autorun = (object *)FSRead32(file);
  file.close();
  if (autorun != NULL) {
    loadimage(NULL);
    apply(autorun, NULL, NULL);
  }
#elif defined(DATAFLASH) || defined(CPUFLASH) || defined(EEPROMFLASH)
  if (!FlashCheck()) error2("flash not available");
  uint32_t addr;
  FlashBeginRead(&addr);
  object *autorun = (object *)FlashRead32(&addr);
  FlashEndRead(&addr);
  if (autorun != NULL && (unsigned int)autorun != 0xFFFFFFFF) {
    loadimage(nil);
    apply(autorun, NULL, NULL);
  }
#else
  error2("autorun not available");
#endif
}

