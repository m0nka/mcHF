/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:       usbh_conf.h                                                  **
**  Description:     USB Host library configuration for bootloader                 **
**  Last Modified:                                                                 **
**  Licence:         https://github.com/m0nka/mcHF/blob/main/LICENSE              **
************************************************************************************/
#ifndef __USBH_CONF_H
#define __USBH_CONF_H

#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------
// USB Host Library configuration — minimal for MSC class only
// -----------------------------------------------------------------------
#define USBH_MAX_NUM_ENDPOINTS                2
#define USBH_MAX_NUM_INTERFACES               2
#define USBH_MAX_NUM_CONFIGURATION            1
#define USBH_MAX_NUM_SUPPORTED_CLASS          1
#define USBH_KEEP_CFG_DESCRIPTOR              0
#define USBH_MAX_SIZE_CONFIGURATION           0x200
#define USBH_MAX_DATA_BUFFER                  0x200
#define USBH_DEBUG_LEVEL                      1       // errors + user logs
#define USBH_USE_OS                           0

// -----------------------------------------------------------------------
// Memory management — bootloader uses libc malloc/free
// -----------------------------------------------------------------------
#define USBH_malloc               malloc
#define USBH_free                 free
#define USBH_memset               memset
#define USBH_memcpy               memcpy

// -----------------------------------------------------------------------
// Debug macros — route through printf (USART2)
// -----------------------------------------------------------------------
#if (USBH_DEBUG_LEVEL > 0)
#define USBH_UsrLog(...)    do { printf(__VA_ARGS__); printf("\r\n"); } while(0)
#else
#define USBH_UsrLog(...)
#endif

#if (USBH_DEBUG_LEVEL > 1)
#define USBH_ErrLog(...)    do { printf("USB ERR: "); printf(__VA_ARGS__); printf("\r\n"); } while(0)
#else
#define USBH_ErrLog(...)
#endif

#if (USBH_DEBUG_LEVEL > 2)
#define USBH_DbgLog(...)    do { printf("USB DBG: "); printf(__VA_ARGS__); printf("\r\n"); } while(0)
#else
#define USBH_DbgLog(...)
#endif

#endif // __USBH_CONF_H
