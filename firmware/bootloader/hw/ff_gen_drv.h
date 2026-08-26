/************************************************************************************
**  ff_gen_drv.h — FatFs generic driver registration (ff16-compatible)            **
**  Based on STMicroelectronics template, adapted for FatFs R0.16                 **
************************************************************************************/
#ifndef __FF_GEN_DRV_H
#define __FF_GEN_DRV_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "ff.h"
#include "diskio.h"
#include "stdint.h"

// Disk IO Driver structure
typedef struct
{
  DSTATUS (*disk_initialize) (BYTE);
  DSTATUS (*disk_status)     (BYTE);
  DRESULT (*disk_read)       (BYTE, BYTE*, DWORD, UINT);
#if FF_FS_READONLY == 0
  DRESULT (*disk_write)      (BYTE, const BYTE*, DWORD, UINT);
#endif
  DRESULT (*disk_ioctl)      (BYTE, BYTE, void*);

}Diskio_drvTypeDef;

// Global Disk IO Drivers structure
typedef struct
{
  uint8_t                 is_initialized[FF_VOLUMES];
  const Diskio_drvTypeDef *drv[FF_VOLUMES];
  uint8_t                 lun[FF_VOLUMES];
  volatile uint8_t        nbr;

}Disk_drvTypeDef;

uint8_t FATFS_LinkDriver(const Diskio_drvTypeDef *drv, char *path);
uint8_t FATFS_UnLinkDriver(char *path);
uint8_t FATFS_LinkDriverEx(const Diskio_drvTypeDef *drv, char *path, BYTE lun);
uint8_t FATFS_UnLinkDriverEx(char *path, BYTE lun);
uint8_t FATFS_GetAttachedDriversNbr(void);

#ifdef __cplusplus
}
#endif

#endif // __FF_GEN_DRV_H
