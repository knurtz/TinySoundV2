#ifndef FLASH_FUNCTIONS_H
#define FLASH_FUNCTIONS_H

#include "pico/stdlib.h"

void Flash_Init(void);

uint32_t Flash_QueueWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize);

void Flash_WriteStartSection(void);
void Flash_WriteCurrentSection(void);

void Flash_WriteCycle(bool forced);

#endif /* FLASH_FUNCTIONS_H */
