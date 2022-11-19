#include "tusb.h"
#include "TS_shell.h"

// Invoked when device is mounted
void tud_mount_cb(void)
{
    
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    
}

// Invoked when cdc when line state changed e.g connected / disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf;
    if (!tud_cdc_connected()) return;
    Shell_Restart();
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
    (void) itf;
}