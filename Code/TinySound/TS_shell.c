#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "TS_shell.h"
#include "TS_audio.h"

static char shell_buffer[64];
static volatile bool shell_overflow = false;
static int dma_chan;


void Shell_BufferOverflow(void)
{
    // Clear the interrupt request
    dma_hw->ints0 = 1u << dma_chan;
    shell_overflow = true;
}

void Shell_Init(void)
{
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

    // Start shell for the first time
    Shell_Restart();
}

void Shell_Restart(void)
{
    // stop any DMA transfers
    dma_channel_abort(dma_chan);

    // clear receive buffer
    memset(shell_buffer, 0, sizeof(shell_buffer));
    shell_overflow = false;
    
    // echo prompt
    printf("TinySound> ");

    // restart DMA transfer
    dma_channel_set_write_addr(dma_chan, shell_buffer, true);
}

bool Shell_CheckCommand(void)
{
    if (strchr(shell_buffer, '\n')) 
    {          
        // play sound command
        if (strstr(shell_buffer, "play"))
        {
            printf("play\n");
        }
        // stop sound command
        else if (strstr(shell_buffer, "stop"))
        {
            printf("stop\n");
        }
        // default: unrecognized command
        else printf("Unknown command: %s", shell_buffer);

        Shell_Restart();
        return true;
    }

    if (shell_overflow)
    {        
        printf("Buffer overflow!\n");
        Shell_Restart();
    }

    return false;
}