#include "tusb.h"
#include "msc_disk.h"
#include "flash_functions.h"
#include "TS_shell.h"

static bool msc_ejected = false;

// Invoked when USB device is mounted
void tud_mount_cb(void) {}

// Invoked when USB device is unmounted
void tud_umount_cb(void) {}

// Invoked when bus is suspended
// remote_wakeup_en: if host allow us  to perform remote wakeup
// Within 7 ms device must draw an average current of less than 2.5 mA
void tud_suspend_cb(bool remote_wakeup_en) {}

// Invoked when bus is resumed
void tud_resume_cb(void) {}

// Invoked when cdc line state changed
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    if (!tud_cdc_connected()) return;
    Shell_Restart();
}

// Invoked when cdc interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {}

// Invoked when received SCSI_CMD_INQUIRY
// Fill vendor id, product id, revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
  const char vid[] = "TS";
  const char pid[] = "TinySound";
  const char rev[] = "0.1";

  memcpy(vendor_id,   vid, strlen(vid));
  memcpy(product_id,  pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// Returning true allows host to read/write this LUN
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  // Ready until msc_ejected
  if (msc_ejected)
  {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return false;
  }

  return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
  *block_count = DISK_SECTOR_NUM;
  *block_size  = DISK_SECTOR_SIZE;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  if (load_eject && !start)
  {
    // Unload disk storage
    msc_ejected = true;
    // Copy data to flash one last time
    Flash_WriteCycle(true);
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  return Flash_ReadQueued(lba, offset, buffer, bufsize);
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
  //xprintf("Write %d %d %d\n", lba, offset, bufsize);

  return Flash_WriteQueued(lba, offset, buffer, bufsize);
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
  void const* response = NULL;
  uint16_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
      // Host is about to read/write etc ... better not to disconnect disk
      resplen = 0;
    break;

    default:
      // Set Sense = Invalid Command Operation
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

      // negative means error -> tinyusb could stall and/or response with failed status
      resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if ( resplen > bufsize ) resplen = bufsize;

  if ( response && (resplen > 0) )
  {
    if(in_xfer)
    {
      memcpy(buffer, response, resplen);
    }else
    {
      // SCSI output
    }
  }

  return resplen;
}