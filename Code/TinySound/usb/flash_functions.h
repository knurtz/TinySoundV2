#ifndef FLASH_FUNCTIONS_H
#define FLASH_FUNCTIONS_H

#include "pico/stdlib.h"

void Flash_Init(void);

uint32_t Flash_ReadQueued(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);
uint32_t Flash_WriteQueued(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);

void Flash_WriteStartSection(void);
void Flash_WriteCurrentSection(void);

void Flash_WriteCycle(bool forced);

extern bool disk_initialized;

#endif /* FLASH_FUNCTIONS_H */
