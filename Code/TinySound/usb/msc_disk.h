#include "pico/stdlib.h"

#define DISK_SECTOR_NUM         2048        // Disk size 1 MB -> 2048 * 512 bytes
#define DISK_SECTOR_SIZE        512
#define DISK_CLUSTER_SIZE       8           // Cluster size 4096 bytes

#define MASS_STORAGE_START  0x10200000

extern const __attribute__((section(".mass_storage")))
uint8_t msc_disk[DISK_SECTOR_NUM][DISK_SECTOR_SIZE];