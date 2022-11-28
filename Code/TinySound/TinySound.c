#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "tusb.h"

#include "TS_shell.h"

uint32_t last_blink_time_ms = 0;
uint16_t blink_interval_ms = 200;

void blink(void);

int main()
{
    stdio_init_all();
    tusb_init();
    Shell_Init();

    gpio_init(28);
    gpio_set_dir(28, GPIO_OUT);

    while (true)
    {
        tud_task();
        Shell_CheckCommand();

        blink();
    }

    return 0;
}

void blink(void)
{
    if (time_us_32() / 1000 > last_blink_time_ms + blink_interval_ms)
    {
        last_blink_time_ms = time_us_32() / 1000;
        gpio_put(28, !gpio_get(28));
    }
}