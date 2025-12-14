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

#define SD_DETECT_BEFORE_OS
#define STORAGE_BSP_INIT

#define FILEMGR_LIST_DEPDTH                    24
#define FILEMGR_FILE_NAME_SIZE                256
#define FILEMGR_MAX_LEVEL                       3
#define FILEMGR_MAX_EXT_SIZE                    3

#define FILETYPE_DIR                            0
#define FILETYPE_FILE                           1

typedef enum
{
  MSD_DISK_UNIT,
  NUM_DISK_UNITS
}
STORAGE_IdTypeDef;

typedef enum
{
  STORAGE_NOINIT
, STORAGE_UNMOUNTED // same as STORAGE_CONNECTED
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

// Structure version madness!!
#if 0
typedef struct _FILELIST_FileTypeDef
{
  FILELIST_LineTypeDef  file[FILEMGR_LIST_DEPDTH] ;
  uint16_t              ptr;
}
FILELIST_FileTypeDef;
#else
typedef struct _FILELIST_FileTypeDef
{
  FILELIST_LineTypeDef  file[FILEMGR_LIST_DEPDTH] ;
  uint16_t              ptr;
  struct _FILELIST_FileTypeDef *next;
  struct _FILELIST_FileTypeDef *prev;
}
FILELIST_FileTypeDef;
#endif

extern char StorageDISK_Drive[];
extern osMessageQId StorageEvent;

void 		storage_proc_task(void const * argument);
void        storage_proc_init(void);

#endif
