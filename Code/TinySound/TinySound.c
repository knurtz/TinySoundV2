#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "tusb.h"
#include "flash_functions.h"

#include "TS_shell.h"
#include "TS_audio.h"
#include "TS_fat.h"


#define LED_PIN         28
uint32_t next_blink_ms = 0;
uint16_t blink_interval_ms = 500;
uint16_t blink_interval_fast_ms = 200;

void blink(void)
{
    if (time_us_32() / 1000 > next_blink_ms)
    {
        next_blink_ms += Audio_IsPlaying() ? blink_interval_fast_ms : blink_interval_ms;
        gpio_put(LED_PIN, !gpio_get(LED_PIN));
    }
}


#define TRIGGER_PIN     5
#define TRIGGER_DELAY 10000      // us
uint32_t last_trigger = 0;      // us
uint8_t trigger_counter = 0;

void __isr __time_critical_func(trigger_callback)(uint gpio, uint32_t events)
{
    trigger_counter++;
    last_trigger = time_us_32();
}


char* tracklist[] =
    {
        "01.wav",
        "02.wav",
        "03.wav",
        "04.wav",
        "05.wav",
        "06.wav",
        "07.wav",
        "08.wav",
        "09.wav"
    };


int main()
{
    stdio_init_all();
    tusb_init();

    Shell_Init();
    Flash_Init();
    Audio_Init();

    // Init LED output
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Init play trigger input
    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_IN);
    gpio_disable_pulls(TRIGGER_PIN);
    gpio_set_irq_enabled_with_callback(TRIGGER_PIN, GPIO_IRQ_EDGE_FALL, true, &trigger_callback);

    FAT_Init();

    while (true)
    {
        tud_task();
        Shell_CheckCommand();
        Flash_WriteCycle(false);
        Audio_CheckBuffer();
        
        if (trigger_counter && time_us_32() > last_trigger + TRIGGER_DELAY)
        {
            xprintf("Triggered %d times\n", trigger_counter);

            if (trigger_counter < count_of(tracklist));
                Audio_Play(tracklist[trigger_counter]);

            trigger_counter = 0;
        }

        blink();
    }

    return 0;
}


void _close(void) {}
void _lseek(void) {}