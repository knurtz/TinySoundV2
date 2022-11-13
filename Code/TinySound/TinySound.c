#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "tusb.h"

#include "TS_shell.h"

void cdc_task();

enum  {
  BLINK_NOT_MOUNTED = 100,
  BLINK_MOUNTED = 500,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

int main()
{
    stdio_init_all();

    //sleep_ms(2000);
    
    printf("Hello, world!\n");

    Shell_Init();

    gpio_init(28);
    gpio_set_dir(28, GPIO_OUT);
    
    tusb_init();

    while (true)
    {
        tud_task();

        cdc_task();

        Shell_CheckCommand();

        /*
        gpio_put(28, 1);
        sleep_ms(blink_interval_ms);
        gpio_put(28, 0);
        sleep_ms(blink_interval_ms);
        */
    }

    return 0;
}

// Invoked when device is mounted
void tud_mount_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
(void) remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

void cdc_task()
{
    if (tud_cdc_connected())
    {
        // connected and there are data available
        if (tud_cdc_available())
        {
            uint8_t buf[64];

            // read and echo back
            uint32_t count = tud_cdc_read(buf, sizeof(buf));

            for(uint32_t i = 0; i < count; i++)
            {
                tud_cdc_write_char(buf[i]);

                if (buf[i] == '\r') tud_cdc_write_char('\n');
            }

            tud_cdc_write_flush();
        }
    }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf;

    // connected
    if (dtr && rts)
    {
        // print initial message when connected
        tud_cdc_write_str("\r\nTinyUSB CDC MSC device example\r\n");
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
    (void) itf;
}