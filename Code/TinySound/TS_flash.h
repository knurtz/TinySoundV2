/*
TinySound flash layout

 - Total accessible flash size:     16M
 - Max. program size:               2M (defined in linker settings, should be changed at a later date once final program size is determined)
 - Mass storage area begins at:     XIP_BASE + 2 * 1024 * 1024

 */

#ifndef TS_FLASH
#define TS_FLASH

#include "hardware/flash.h"

#define MASS_STORAGE_BASE           XIP_BASE + 2 * 1024 * 1024



#endif /* TS_FLASH */