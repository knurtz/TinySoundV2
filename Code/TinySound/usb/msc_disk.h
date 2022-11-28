#include "pico/stdlib.h"

enum
{
  DISK_BLOCK_NUM  = 2048,       // Disk size 1 MB
  DISK_BLOCK_SIZE = 512
};

extern const __attribute__((section(".mass_storage")))
uint8_t msc_disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE];