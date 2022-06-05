#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "TS_shell.h"

int main()
{
    stdio_init_all();
    
    printf("Hello, world!\n");

    Shell_Init();

    gpio_init(28);
    gpio_set_dir(28, GPIO_OUT);

    while (true)
    {
        Shell_CheckCommand();

        gpio_put(28, 1);
        sleep_ms(100);
        gpio_put(28, 0);
        sleep_ms(100);
    }

    return 0;
}
