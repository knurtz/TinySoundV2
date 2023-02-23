#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "tusb.h"
#include "flash_functions.h"

#include "TS_shell.h"
#include "TS_audio.h"
#include "TS_fat.h"


uint32_t next_blink_ms = 0;
uint16_t blink_interval_ms = 200;
uint16_t blink_interval_fast_ms = 100;

void blink(void)
{
    if (time_us_32() / 1000 > next_blink_ms)
    {
        next_blink_ms += Audio_IsPlaying() ? blink_interval_fast_ms : blink_interval_ms;
        gpio_put(28, !gpio_get(28));
    }
}


int main()
{
    stdio_init_all();
    tusb_init();

    Shell_Init();
    Flash_Init();
    Audio_Init();

    // Init LED output (GPIO3)
    gpio_init(28);
    gpio_set_dir(28, GPIO_OUT);

    // Init play trigger input (GPIO2)
    gpio_init(18);
    gpio_set_dir(18, GPIO_IN);
    gpio_pull_up(18);

    FAT_Init();
    //Audio_Play("preuss~1.wav");

    while (true)
    {
        tud_task();
        Shell_CheckCommand();
        Flash_WriteCycle(false);
        Audio_CheckBuffer();

        if (!gpio_get(18) && !Audio_IsPlaying())
        {
            Audio_Play("moep.wav");
        }

        blink();
    }

    return 0;
}


void _close(void) {}
void _lseek(void) {}