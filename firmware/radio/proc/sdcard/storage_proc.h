/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:                                                                       **
************************************************************************************/
#ifndef __STORAGE_PROC_H
#define __STORAGE_PROC_H

#if defined(USE_USB_FS) || defined(USE_USB_HS)
#include "usbh_diskio.h"
#endif /* USE_USB_FS | USE_USB_HS */

#define FILEMGR_LIST_DEPDTH                    24
#define FILEMGR_FILE_NAME_SIZE                256
#define FILEMGR_MAX_LEVEL                       3
#define FILEMGR_MAX_EXT_SIZE                    3

#define FILETYPE_DIR                            0
#define FILETYPE_FILE                           1

typedef enum
{
#if defined(USE_SDCARD)
  MSD_DISK_UNIT ,
#endif /* USE_SDCARD */
#if defined(USE_USB_FS) || defined(USE_USB_HS)
  USB_DISK_UNIT ,
#endif /* USE_USB_FS | USE_USB_HS */
  NUM_DISK_UNITS
}
STORAGE_IdTypeDef;

typedef enum
{
  STORAGE_NOINIT
, STORAGE_UNMOUNTED /* same as STORAGE_CONNECTED */
, STORAGE_MOUNTED
} STORAGE_Status_t;

typedef enum
{
  USBDISK_DISCONNECTION_EVENT = 1,
  USBDISK_CONNECTION_EVENT,
  USBDISK_ACTIVE_CLASS_EVENT,
  MSDDISK_DISCONNECTION_EVENT,
  MSDDISK_CONNECTION_EVENT,
}
STORAGE_EventTypeDef;

typedef struct _FILELIST_LineTypeDef
{
  uint8_t               type;
  uint8_t               name[FILEMGR_FILE_NAME_SIZE];

}
FILELIST_LineTypeDef;

typedef struct _FILELIST_FileTypeDef
{
  FILELIST_LineTypeDef  file[FILEMGR_LIST_DEPDTH] ;
  uint16_t              ptr;
}
FILELIST_FileTypeDef;

void        Storage_Init(void);
void        Storage_DeInit(void);
uint8_t     Storage_GetStatus (uint8_t unit);
const char *Storage_GetDrive (uint8_t unit);

extern osMessageQId StorageEvent;

#endif
