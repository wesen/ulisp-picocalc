// ulisp_persistence.h — filename helpers and image save/load backends.

#ifndef ULISP_PERSISTENCE_H
#define ULISP_PERSISTENCE_H

#include <Arduino.h>
#include "ulisp_types.h"
#if defined(sdcardsupport)
#include <SD.h>
#endif

char *MakeFilename(object *arg, char *buffer);
void SDBegin();
#if defined(sdcardsupport)
void SDWrite32(File file, int data);
int SDRead32(File file);
void FSWrite32(File file, uint32_t data);
uint32_t FSRead32(File file);
#endif
void FlashBusy();
void FlashWrite(uint8_t data);
uint8_t FlashReadByte();
void FlashWriteByte(uint32_t *addr, uint8_t data);
void FlashWriteEnable();
bool FlashCheck();
void FlashBeginWrite(uint32_t *addr, uint32_t bytes);
void FlashWrite32(uint32_t *addr, uint32_t data);
void FlashEndWrite(uint32_t *addr);
void FlashBeginRead(uint32_t *addr);
uint32_t FlashRead32(uint32_t *addr);
void FlashEndRead(uint32_t *addr);
void row_erase(const volatile void *addr);
void page_clear();
void page_write();
int saveimage(object *arg);
int loadimage(object *arg);
void autorunimage();

#endif // ULISP_PERSISTENCE_H
