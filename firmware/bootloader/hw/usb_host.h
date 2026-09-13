/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:       usb_host.h                                                   **
**  Description:     USB Host MSC support for bootloader                           **
**  Last Modified:                                                                 **
**  Licence:         https://github.com/m0nka/mcHF/blob/main/LICENSE              **
************************************************************************************/
#ifndef __USB_HOST_H
#define __USB_HOST_H

#include "mchf_pro_board.h"

// -----------------------------------------------------------------------
// USB host state
// -----------------------------------------------------------------------
#define USB_STATE_IDLE          0
#define USB_STATE_READY         1
#define USB_STATE_DISCONNECTED  2
#define USB_STATE_ERROR         3

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

// Initialise USB OTG_HS in host mode and register MSC class
// Returns 0 on success
uchar usb_host_init(void);

// Call repeatedly in a polling loop to advance the USB state machine
void usb_host_process(void);

// Block until a USB MSC device is ready, or timeout_ms expires
// Returns 0 if device ready, non-zero on timeout/error
uchar usb_host_wait_ready(ulong timeout_ms);

// Current state (USB_STATE_xxx)
uchar usb_host_get_state(void);

// Shut down USB host peripheral and release resources
void usb_host_deinit(void);

#endif // __USB_HOST_H
