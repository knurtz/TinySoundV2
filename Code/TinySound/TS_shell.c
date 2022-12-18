#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "tusb.h"

#include "TS_shell.h"
#include "TS_audio.h"

#include "ff.h"

FATFS fs;

static char shell_buffer[64];
static volatile bool shell_overflow = false;
static bool fs_mounted = false;


void ListFolder(const char* dir_name, uint8_t current_depth)
{
    DIR dir;
    FILINFO fileinfo;
    FRESULT res;

    res = f_opendir(&dir, dir_name);
    if (res) xprintf("opendir - Error %d\n", res);
    res = f_readdir(&dir, &fileinfo);
    if (res) xprintf("readdir - Error %d\n", res);

    while (fileinfo.fname[0])
    {
        xprintf("%*s%s\n", current_depth * 2, "", fileinfo.fname);
        if (fileinfo.fattrib & AM_DIR) 
        {
            size_t next_dir_length = strlen(dir_name) + strlen(fileinfo.altname) + 3;
            char next_dir[next_dir_length];
            snprintf(next_dir, next_dir_length - 1, "%s/%s", dir_name, fileinfo.altname);
            ListFolder(next_dir, current_depth + 1);
        }
        res = f_readdir(&dir, &fileinfo);
        if (res) xprintf("readdir - Error %d\n", res);
    }
}


void Shell_BufferOverflow(void)
{
    shell_overflow = true;
}

void Shell_Init(void)
{
    Shell_Restart();
}

void Shell_Restart(void)
{
   // clear receive buffer
    memset(shell_buffer, 0, sizeof(shell_buffer));
    shell_overflow = false;
    
    // echo prompt
    xprintf("\nTinySound> ");
}

bool Shell_CheckCommand(void)
{
    if (!tud_cdc_connected()) return false;

    // Receive from CDC port
    if (tud_cdc_available())
    {
        size_t len = strlen(shell_buffer);
        tud_cdc_read(shell_buffer + len, sizeof(shell_buffer) - 1 - len);

        // if there is still more data to receive, trigger buffer overflow
        if (tud_cdc_available()) shell_overflow = true;
    }

    if (strchr(shell_buffer, '\n')) 
    {
        // Echo executed command
        tud_cdc_write_str(shell_buffer);

        // play sound command
        if (strstr(shell_buffer, "play"))
        {
            xprintf("play");
        }

        // stop sound command
        else if (strstr(shell_buffer, "stop"))
        {
            xprintf("stop");
        }

        // mount filesystem command
        else if (strstr(shell_buffer, "mount"))
        {
            FRESULT res = f_mount(&fs, "", 1);
            if (res == FR_OK) 
            {
                char vol_name[64];
                f_getlabel("", vol_name, NULL);
                xprintf("Mount successful: %s", vol_name);
                fs_mounted = true;
            }
            else xprintf("Error: %d", res);
        }

        // show tree command
        else if (strstr(shell_buffer, "tree"))
        {
            if (fs_mounted) ListFolder("/", 0);
        }

        // show file contents
        else if (strstr(shell_buffer, "show "))
        {
            if (fs_mounted && strlen(shell_buffer) > 5)
            {
                FIL f;
                FRESULT res = f_open(&f, shell_buffer + 5, FA_READ);
                if (res) xprintf("Error: %d", res);
                else
                {
                    char read_text[32];
                    UINT rb;
                    f_read(&f, read_text, 31, &rb);
                    read_text[rb] = '\0';
                    xprintf("%s\n", read_text);
                }
            }
        }

        // default
        else xprintf("Unknown command");

        Shell_Restart();
        return true;
    }

    //tud_cdc_write_flush();

    if (shell_overflow)
    {        
        xprintf("Buffer overflow!");
        Shell_Restart();
    }

    tud_cdc_write_flush();

    return false;
}


// Works like normal printf, max. length 150 characters
void xprintf(const char *fmt, ...)
{
    if (!tud_cdc_connected()) return;

    static char buffer[151];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    tud_cdc_write_str(buffer);
    tud_cdc_write_flush();
    tud_task();
}