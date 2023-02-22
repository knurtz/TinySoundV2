#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "tusb.h"

#include "TS_shell.h"
#include "TS_fat.h"
#include "TS_audio.h"

static char shell_buffer[64];
static volatile bool shell_overflow = false;

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

        // show tree command
        else if (strstr(shell_buffer, "tree"))
        {
            FAT_ListFolder("/", 0);
        }

        // show file contents
        else if (strstr(shell_buffer, "show "))
        {
            //FAT_PrintFile(shell_buffer + 5, 64);
            char buf[12];
            FAT_ReadFileToBuffer(shell_buffer + 5, 3, 10, buf);
            buf[11] = '\0';
            xprintf("%s", buf);
        }

        // test audio
        else if (strstr(shell_buffer, "audio "))
        {
            Audio_Play(shell_buffer + 6);
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