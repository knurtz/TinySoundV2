#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

char shell_buffer[64];
bool shell_overflow = false;
int dma_chan;


#include "mass_storage_init.c"


void Shell_Init(void);
void Shell_BufferOverflow(void);
void Shell_Restart(void);
bool Shell_CheckCommand(void);


void Shell_BufferOverflow(void)
{
    // Clear the interrupt request.
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
    channel_config_set_dreq(&c, DREQ_UART1_RX);

    for (size_t i = 0; i < sizeof(mass_storage_data); i++)
    {
        printf("foo %i", mass_storage_data[i]);
    }

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

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
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
    
    // echo promt
    printf("TinySound> ");

    // restart DMA transfer
    dma_channel_set_write_addr(dma_chan, shell_buffer, true);
}

bool Shell_CheckCommand(void)
{  
    // test with simple echo after newline
    if (strchr(shell_buffer, '\n')) printf(shell_buffer);
    Shell_Restart();
    return true;

    if (shell_overflow)
    {        
        printf("Buffer overflow!\n");
        Shell_Restart();
    }
}

int main()
{
    stdio_init_all();
    printf("Hello, world!\n");

    Shell_Init();

    while (true)
    {
        Shell_CheckCommand();
    }

    return 0;
}
