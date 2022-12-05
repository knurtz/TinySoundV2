#include "tusb.h"
#include "msc_disk.h"
#include "flash_functions.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "TS_shell.h"

// Keep the first section of 8 sectors permanently in RAM and only write to flash periodically and on eject.
// This section contains FAT and root directory and are overwritten many times.
uint8_t flash_start[8][512];
bool flash_start_modified = false;

// Another variable holds a copy of the section that has been modified last.
// Only once a different section is accessed, the flash will be written.
uint8_t flash_section[8][512];
uint32_t current_section = 0;
uint8_t modified_sectors = 0;

// If any changes remain, they are periodically written to flash
uint32_t last_write_time_ms = 0;
uint16_t write_interval_ms = 10000;

void Flash_Init(void)
{
    // Initialize the start section
    memcpy(flash_start, msc_disk, sizeof(flash_start));
}

uint32_t Flash_ReadQueued(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    if (offset != 0 || bufsize != 512) return 0;

    uint32_t target_section = lba / 8;
    uint8_t target_sector = lba % 8;

    // Get data for first section from flash_start[] in RAM
    if (target_section == 0)
    {
        memcpy(buffer, flash_start[target_sector], bufsize);
    }

    // Get data from flash_section[] (also in RAM), if modified sector has not yet been written to flash
    else if (target_section == current_section && modified_sectors & (1 << target_sector))
    {
        memcpy(buffer, flash_section[target_sector], bufsize);
    }

    // Get all other sectors from flash
    else
    {
        memcpy(buffer, msc_disk[lba], bufsize);
    }

    return bufsize;
}

uint32_t Flash_QueueWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    if (offset != 0 || bufsize != 512) return 0;

    uint32_t target_section = lba / 8;
    uint8_t target_sector = lba % 8;

    if (target_section == 0)
    {            
        //for (uint32_t start_addr = lba * 512 + offset; start_addr < lba * 512 + bufsize; start_addr += 512)

        //xprintf("Queue %d in start section\n", lba);
        memcpy(flash_start[target_sector], buffer, bufsize);
        flash_start_modified = true;

        return bufsize;
    }

    else
    {   
        // If data needs to be written to a different section than last time, now is the time to push the pending changes to flash
        if (target_section != current_section)
        {
            Flash_WriteCurrentSection();
        }

        current_section = target_section;
        //xprintf("Queue %d in section %d\n", target_sector, target_section);
        memcpy(flash_section[target_sector], buffer, bufsize);
        modified_sectors |= 1 << target_sector;

        return bufsize;
    }
}

void Flash_WriteStartSection(void)
{
    if (!flash_start_modified) return;
    //xprintf(" Write start section\n");

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase((uint32_t)msc_disk - XIP_BASE, 8 * 512);
    flash_range_program((uint32_t)msc_disk - XIP_BASE, (uint8_t*)flash_start, 8 * 512);
    restore_interrupts (ints);

    flash_start_modified = false;
}

void Flash_WriteCurrentSection(void)
{
    if (!modified_sectors) return;
    //xprintf(" Write section %d\n", current_section);

    // Fill in all sectors that haven't been modified, as they will be overwritten
    for (uint8_t sector = 0; sector < 8; sector++)
    {
        if (!(modified_sectors & (1 << sector)))
        {
            memcpy(flash_section[sector], msc_disk[current_section * 8 + sector], 512);
            //xprintf("  Fill %d\n", sector);
        }
    }

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase((uint32_t)(msc_disk[current_section * 8]) - XIP_BASE, 8 * 512);
    flash_range_program((uint32_t)(msc_disk[current_section * 8]) - XIP_BASE, (uint8_t*)flash_section, 8 * 512);
    restore_interrupts (ints);

    modified_sectors = 0;
}

// Should be periodically called
void Flash_WriteCycle(bool forced)
{
    if (!forced && time_us_32() / 1000 < last_write_time_ms + write_interval_ms) return;
    last_write_time_ms = time_us_32() / 1000;

    //xprintf("\nPeriodic write cycle\n");

    Flash_WriteStartSection();
    Flash_WriteCurrentSection();
}