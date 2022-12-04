#include "pico/stdlib.h"

#define DISK_BLOCK_NUM      2048       // Disk size 1 MB -> 2048 * 512
#define DISK_BLOCK_SIZE     512

#define MASS_STORAGE_START  0x10200000

extern const __attribute__((section(".mass_storage")))
uint8_t msc_disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE];