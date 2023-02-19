#ifndef TS_FAT_H
#define TS_FAT_H

#include "pico/stdlib.h"

bool FAT_Init(void);
void FAT_ListFolder(const char* dir_name, uint8_t current_depth);
void FAT_PrintFile(const char* filename, uint32_t len);

#endif /* TS_FAT_H */
