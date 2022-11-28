#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "tusb.h"

#include "TS_shell.h"
#include "TS_audio.h"

static char shell_buffer[64];
static volatile bool shell_overflow = false;
//static int dma_chan;

// Test memory locations

char test0 = 40;                                                        // RAM -> address bigger than 0x20000000
const char test1 = 50;                                                  // ??
const char __in_flash() test2 = 60;                                     // Flash -> address between 0x10000000 and 0x10200000
const __attribute__((section(".flashdata"))) char test3 = 70;           // Alias for the above
const __attribute__((section(".mass_storage"))) char test4 = 80;        // Mass storage -> address bigger than or equal 0x10200000


void Shell_BufferOverflow(void)
{
    // Clear the interrupt request
    //dma_hw->ints0 = 1u << dma_chan;
    shell_overflow = true;
}

void Shell_Init(void)
{
    /*
    // Init DMA channel
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_UART1_RX);

    dma_channel_configure(
        dma_chan,
        &c,
        shell_buffer,                   // Write to receive buffer
        &uart1_hw->dr,                  // Read from UART1
        sizeof(shell_buffer) - 1,       // Read until buffer is full (keep at least one trailing \0 to properly end string)
        false                           // Don't start yet
    );

    // Tell DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dma_chan, true);

    // Run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, Shell_BufferOverflow);
    irq_set_enabled(DMA_IRQ_0, true);
    */

    // Start shell for the first time
    Shell_Restart();
}

void Shell_Restart(void)
{
    // stop any DMA transfers
    //dma_channel_abort(dma_chan);

    // clear receive buffer
    memset(shell_buffer, 0, sizeof(shell_buffer));
    shell_overflow = false;
    
    // echo prompt
    xprintf("\nTinySound> ");

    // restart DMA transfer
    //dma_channel_set_write_addr(dma_chan, shell_buffer, true);
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

        // test addresses command
        else if (strstr(shell_buffer, "memory"))
        {
            xprintf("test4: %d @ 0x%x\n", test4, &test4);
            xprintf("test0: %d @ 0x%x\n", test0, &test0);
            xprintf("test1: %d @ 0x%x\n", test1, &test1);
            xprintf("test2: %d @ 0x%x\n", test2, &test2);
            xprintf("test3: %d @ 0x%x\n", test3, &test3);
        }

        // default: unrecognized command
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